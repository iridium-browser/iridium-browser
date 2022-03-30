import datetime
import fnmatch
import logging
import os
import os.path
import Queue
import sublime
import sublime_plugin
import subprocess
import tempfile
import threading
import time


class CompileCurrentFile(sublime_plugin.TextCommand):
  # static thread so that we don't try to run more than once at a time.
  thread = None
  lock = threading.Lock()

  def __init__(self, args):
    super(CompileCurrentFile, self).__init__(args)
    self.thread_id = threading.current_thread().ident
    self.text_to_draw = ""
    self.interrupted = False

  def description(self):
    return ("Compiles the file in the current view using Ninja, so all that "
            "this file and it's project depends on will be built first\n"
            "Note that this command is a toggle so invoking it while it runs "
            "will interrupt it.")

  def draw_panel_text(self):
    """Draw in the output.exec panel the text accumulated in self.text_to_draw.

    This must be called from the main UI thread (e.g., using set_timeout).
    """
    assert self.thread_id == threading.current_thread().ident
    logging.debug("draw_panel_text called.")
    self.lock.acquire()
    text_to_draw = self.text_to_draw
    self.text_to_draw = ""
    self.lock.release()

    if len(text_to_draw):
      edit = self.output_panel.begin_edit()
      self.output_panel.set_read_only(False)
      self.output_panel.insert(edit, self.output_panel.size(), text_to_draw)
      self.output_panel.show(self.output_panel.size())
      self.output_panel.set_read_only(True)
      self.output_panel.end_edit(edit)
      self.view.window().run_command("show_panel", {"panel": "output.exec"})
      logging.debug("Added text:\n%s.", text_to_draw)

  def update_panel_text(self, text_to_draw):
    self.lock.acquire()
    self.text_to_draw += text_to_draw
    self.lock.release()
    sublime.set_timeout(self.draw_panel_text, 0)

  def execute_command(self, command):
    """Execute the provided command and send ouput to panel.

    Because the implementation of subprocess can deadlock on windows, we use
    a Queue that we write to from another thread to avoid blocking on IO.

    Args:
      command: A list containing the command to execute and it's arguments.
    Returns:
      The exit code of the process running the command or,
       1 if we got interrupted.
      -1 if we couldn't start the process
      -2 if we couldn't poll the running process
    """
    logging.debug("Running command: %s", command)

    def EnqueueOutput(out, queue):
      """Read all the output from the given handle and insert it into the queue.

      Args:
        queue: The Queue object to write to.
      """
      while True:
        # This readline will block until there is either new input or the handle
        # is closed. Readline will only return None once the handle is close, so
        # even if the output is being produced slowly, this function won't exit
        # early.
        # The potential dealock here is acceptable because this isn't run on the
        # main thread.
        data = out.readline()
        if not data:
          break
        queue.put(data, block=True)
      out.close()

    try:
      proc = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True,
                              stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
    except OSError, e:
      logging.exception('Execution of %s raised exception: %s.', (command, e))
      return -1

    # Use a Queue to pass the text from the reading thread to this one.
    stdout_queue = Queue.Queue()
    stdout_thread = threading.Thread(target=EnqueueOutput,
                                     args=(proc.stdout, stdout_queue))
    stdout_thread.daemon = True  # Ensure this exits if the parent dies
    stdout_thread.start()

    # We use the self.interrupted flag to stop this thread.
    while not self.interrupted:
      try:
        exit_code = proc.poll()
      except OSError, e:
        logging.exception('Polling execution of %s raised exception: %s.',
                          command, e)
        return -2

      # Try to read output content from the queue
      current_content = ""
      for _ in range(2048):
        try:
          current_content += stdout_queue.get_nowait()
        except Queue.Empty:
          break
      self.update_panel_text(current_content)
      current_content = ""
      if exit_code is not None:
        while stdout_thread.isAlive() or not stdout_queue.empty():
          try:
            current_content += stdout_queue.get(block=True, timeout=1)
          except Queue.Empty:
            # Queue could still potentially contain more input later.
            pass
        time_length = datetime.datetime.now() - self.start_time
        self.update_panel_text("%s\nDone!\n(%s seconds)" %
                               (current_content, time_length.seconds))
        return exit_code
      # We sleep a little to give the child process a chance to move forward
      # before we poll it again.
      time.sleep(0.1)

    # If we get here, it's because we were interrupted, kill the process.
    proc.terminate()
    return 1

  def run(self, edit, target_build):
    """The method called by Sublime Text to execute our command.

    Note that this command is a toggle, so if the thread is are already running,
    calling run will interrupt it.

    Args:
      edit: Sumblime Text specific edit brace.
      target_build: Release/Debug/Other... Used for the subfolder of out.
    """
    # There can only be one... If we are running, interrupt and return.
    if self.thread and self.thread.is_alive():
      self.interrupted = True
      self.thread.join(5.0)
      self.update_panel_text("\n\nInterrupted current command:\n%s\n" % command)
      self.thread = None
      return

    # It's nice to display how long it took to build.
    self.start_time = datetime.datetime.now()
    # Output our results in the same panel as a regular build.
    self.output_panel = self.view.window().get_output_panel("exec")
    self.output_panel.set_read_only(True)
    self.view.window().run_command("show_panel", {"panel": "output.exec"})
    # TODO(mad): Not sure if the project folder is always the first one... ???
    project_folder = self.view.window().folders()[0]
    self.update_panel_text("Compiling current file %s\n" %
                           self.view.file_name())
    # The file must be somewhere under the project folder...
    if (project_folder.lower() !=
        self.view.file_name()[:len(project_folder)].lower()):
      self.update_panel_text(
          "ERROR: File %s is not in current project folder %s\n" %
              (self.view.file_name(), project_folder))
    else:
      # Look for a .ninja file that contains our current file.
      logging.debug("Searching for Ninja target")
      file_relative_path = self.view.file_name()[len(project_folder) + 1:]
      output_dir = "%s\\out\\%s" % (project_folder, target_build)
      matches = []
      for root, dirnames, filenames in os.walk(output_dir):
        for filename in fnmatch.filter(filenames, '*.ninja'):
            matches.append(os.path.join(root, filename))
      logging.debug("Found %d Ninja targets", len(matches))
      # The project file name is needed to construct the full Ninja target path.
      project_file = None
      for ninja_file in matches:
        for line in open(ninja_file):
          if file_relative_path in line:
            project_file = os.path.basename(ninja_file)
            break
      if project_file is None:
        self.update_panel_text(
            "ERROR: File %s is not in any Ninja file under %s" %
                (file_relative_path, output_dir))
      else:
        filename = os.path.splitext(os.path.basename(file_relative_path))[0]
        target = "obj\\%s\\%s.%s.obj" % (os.path.dirname(file_relative_path),
                                         os.path.splitext(project_file)[0],
                                         filename)
        command = [
            "ninja", "-C", "%s\\out\\%s" % (project_folder, target_build),
            target]
        self.interrupted = False
        self.thread = threading.Thread(target=self.execute_command,
                                       kwargs={"command":command})
        self.thread.start()

    time_length = datetime.datetime.now() - self.start_time
    logging.debug("Took %s seconds on UI thread to startup",
                  time_length.seconds)
    self.view.window().focus_view(self.view)
