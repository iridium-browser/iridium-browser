# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

r"""A database of OWNERS files.

OWNERS files indicate who is allowed to approve changes in a specific directory
(or who is allowed to make changes without needing approval of another OWNER).
Note that all changes must still be reviewed by someone familiar with the code,
so you may need approval from both an OWNER and a reviewer in many cases.

The syntax of the OWNERS file is, roughly:

lines      := (\s* line? \s* comment? \s* "\n")*

line       := directive
           | "per-file" \s+ glob \s* "=" \s* directive

directive  := "set noparent"
           |  "file:" owner_file
           |  email_address
           |  "*"

glob       := [a-zA-Z0-9_-*?]+

comment    := "#" [^"\n"]*

owner_file := "OWNERS"
           |  [^"\n"]* "_OWNERS"

Email addresses must follow the foo@bar.com short form (exact syntax given
in BASIC_EMAIL_REGEXP, below). Filename globs follow the simple unix
shell conventions, and relative and absolute paths are not allowed (i.e.,
globs only refer to the files in the current directory).

If a user's email is one of the email_addresses in the file, the user is
considered an "OWNER" for all files in the directory.

If the "per-file" directive is used, the line only applies to files in that
directory that match the filename glob specified.

If the "set noparent" directive used, then only entries in this OWNERS file
apply to files in this directory; if the "set noparent" directive is not
used, then entries in OWNERS files in enclosing (upper) directories also
apply (up until a "set noparent is encountered").

If "per-file glob=set noparent" is used, then global directives are ignored
for the glob, and only the "per-file" owners are used for files matching that
glob.

If the "file:" directive is used, the referred to OWNERS file will be parsed and
considered when determining the valid set of OWNERS. If the filename starts with
"//" it is relative to the root of the repository, otherwise it is relative to
the current file. The referred to file *must* be named OWNERS or end in a suffix
of _OWNERS.

Examples for all of these combinations can be found in tests/owners_unittest.py.
"""

import collections
import fnmatch
import os
import random
import re

try:
  # This fallback applies for all versions of Python before 3.3
  import collections.abc as collections_abc
except ImportError:
  import collections as collections_abc


# If this is present by itself on a line, this means that everyone can review.
EVERYONE = '*'


# Recognizes 'X@Y' email addresses. Very simplistic.
BASIC_EMAIL_REGEXP = r'^[\w\-\+\%\.]+\@[\w\-\+\%\.]+$'


# Key for global comments per email address. Should be unlikely to be a
# pathname.
GLOBAL_STATUS = '*'

# Returned if there is no owner and anyone +1
ANYONE='<anyone>'


def _assert_is_collection(obj):
  assert not isinstance(obj, str)
  # Module 'collections' has no 'Iterable' member
  # pylint: disable=no-member
  if hasattr(collections_abc, 'Iterable') and hasattr(collections_abc, 'Sized'):
    assert (isinstance(obj, collections_abc.Iterable) and
            isinstance(obj, collections_abc.Sized))


class SyntaxErrorInOwnersFile(Exception):
  def __init__(self, path, lineno, msg):
    super(SyntaxErrorInOwnersFile, self).__init__((path, lineno, msg))
    self.path = path
    self.lineno = lineno
    self.msg = msg

  def __str__(self):
    return '%s:%d syntax error: %s' % (self.path, self.lineno, self.msg)


class Database(object):
  """A database of OWNERS files for a repository.

  This class allows you to find a suggested set of reviewers for a list
  of changed files, and see if a list of changed files is covered by a
  list of reviewers."""

  def __init__(self, root, fopen, os_path):
    """Args:
      root: the path to the root of the Repository
      open: function callback to open a text file for reading
      os_path: module/object callback with fields for 'abspath', 'dirname',
          'exists', 'join', and 'relpath'
    """
    self.root = root
    self.fopen = fopen
    self.os_path = os_path

    # Pick a default email regexp to use; callers can override as desired.
    self.email_regexp = re.compile(BASIC_EMAIL_REGEXP)

    # Replacement contents for the given files. Maps the file name of an
    # OWNERS file (relative to root) to an iterator returning the replacement
    # file contents.
    self.override_files = {}

    # Mapping of owners to the paths or globs they own.
    self._owners_to_paths = {EVERYONE: set()}

    # Mappings of directories -> globs in the directory -> owners
    # Example: "chrome/browser" -> "chrome/browser/*.h" -> ("john", "maria")
    # Paths used as keys must use slashes as the separator, even on Windows.
    self._paths_to_owners = {}

    # Mapping reviewers to the preceding comment per file in the OWNERS files.
    self.comments = {}

    # Cache of compiled regexes for _fnmatch()
    self._fnmatch_cache = {}

    # Sets of paths that stop us from looking above them for owners.
    # (This is implicitly true for the root directory).
    #
    # The implementation is a mapping:
    # Directory -> globs in the directory,
    #
    # Example:
    # 'ui/events/devices/mojo' -> 'ui/events/devices/mojo/*_struct_traits*.*'
    self._stop_looking = {'': set([''])}

    # Set of files which have already been read.
    self.read_files = set()

    # Set of files which were included from other files. Files are processed
    # differently depending on whether they are regular owners files or
    # being included from another file.
    self._included_files = {}

    # File with global status lines for owners.
    self._status_file = None

  def _file_affects_ownership(self, path):
    """Returns true if the path refers to a file that could affect ownership."""
    filename = self.os_path.split(path)[-1]
    return filename == 'OWNERS' or filename.endswith('_OWNERS')


  def reviewers_for(self, files, author):
    """Returns a suggested set of reviewers that will cover the files.

    files is a sequence of paths relative to (and under) self.root.
    If author is nonempty, we ensure it is not included in the set returned
    in order avoid suggesting the author as a reviewer for their own changes."""
    self._check_paths(files)
    self.load_data_needed_for(files)

    suggested_owners = self._covering_set_of_owners_for(files, author)
    if EVERYONE in suggested_owners:
      if len(suggested_owners) > 1:
        suggested_owners.remove(EVERYONE)
      else:
        suggested_owners = set([ANYONE])
    return suggested_owners

  def files_not_covered_by(self, files, reviewers):
    """Returns the files not owned by one of the reviewers.

    Args:
        files is a sequence of paths relative to (and under) self.root.
        reviewers is a sequence of strings matching self.email_regexp.
    """
    self._check_paths(files)
    self._check_reviewers(reviewers)
    self.load_data_needed_for(files)

    return set(f for f in files if not self.is_covered_by(f, reviewers))

  def _check_paths(self, files):
    def _is_under(f, pfx):
      return self.os_path.abspath(self.os_path.join(pfx, f)).startswith(pfx)
    _assert_is_collection(files)
    assert all(not self.os_path.isabs(f) and
                _is_under(f, self.os_path.abspath(self.root)) for f in files)

  def _check_reviewers(self, reviewers):
    _assert_is_collection(reviewers)
    assert all(self.email_regexp.match(r) for r in reviewers), reviewers

  def is_covered_by(self, objname, reviewers):
    reviewers = list(reviewers) + [EVERYONE]
    while True:
      for reviewer in reviewers:
        for owned_pattern in self._owners_to_paths.get(reviewer, set()):
          if fnmatch.fnmatch(objname, owned_pattern):
            return True
      if self._should_stop_looking(objname):
        break
      objname = self.os_path.dirname(objname)
    return False

  def enclosing_dir_with_owners(self, objname):
    """Returns the innermost enclosing directory that has an OWNERS file."""
    dirpath = objname
    while not self._owners_for(dirpath):
      if self._should_stop_looking(dirpath):
        break
      dirpath = self.os_path.dirname(dirpath)
    return dirpath

  def load_data_needed_for(self, files):
    self._read_global_comments()
    visited_dirs = set()
    for f in files:
      # Always use slashes as separators.
      f = f.replace(os.sep, '/')
      dirpath = self.os_path.dirname(f)
      while dirpath not in visited_dirs:
        visited_dirs.add(dirpath)

        obj_owners = self._owners_for(dirpath)
        if obj_owners:
          break
        self._read_owners(self.os_path.join(dirpath, 'OWNERS'))
        if self._should_stop_looking(dirpath):
          break

        dirpath = self.os_path.dirname(dirpath)

  def _should_stop_looking(self, objname):
    dirname = objname
    while True:
      if dirname in self._stop_looking:
        if any(self._fnmatch(objname, stop_looking)
               for stop_looking in self._stop_looking[dirname]):
          return True
      up_dirname = self.os_path.dirname(dirname)
      if up_dirname == dirname:
        break
      dirname = up_dirname
    return False

  def _get_root_affected_dir(self, obj_name):
    """Returns the deepest directory/path that is affected by a file pattern
    |obj_name|."""
    root_affected_dir = obj_name
    while '*' in root_affected_dir:
      root_affected_dir = self.os_path.dirname(root_affected_dir)
    return root_affected_dir

  def _owners_for(self, objname):
    obj_owners = set()

    # Possibly relevant rules can be found stored at every directory
    # level so iterate upwards, looking for them.
    dirname = objname
    while True:
      dir_owner_rules = self._paths_to_owners.get(dirname)
      if dir_owner_rules:
        for owned_path, path_owners in dir_owner_rules.items():
          if self._fnmatch(objname, owned_path):
            obj_owners |= path_owners
      up_dirname = self.os_path.dirname(dirname)
      if up_dirname == dirname:
        break
      dirname = up_dirname

    return obj_owners

  def _read_owners(self, path):
    owners_path = self.os_path.join(self.root, path)
    if not (self.os_path.exists(owners_path) or (path in self.override_files)):
      return

    if owners_path in self.read_files:
      return

    self.read_files.add(owners_path)

    is_toplevel = path == 'OWNERS'

    comment = []
    dirpath = self.os_path.dirname(path)
    in_comment = False
    # We treat the beginning of the file as an blank line.
    previous_line_was_blank = True
    reset_comment_after_use = False
    lineno = 0

    if path in self.override_files:
      file_iter = self.override_files[path]
    else:
      file_iter = self.fopen(owners_path)

    for line in file_iter:
      lineno += 1
      line = line.strip()
      if line.startswith('#'):
        if is_toplevel:
          m = re.match(r'#\s*OWNERS_STATUS\s+=\s+(.+)$', line)
          if m:
            self._status_file = m.group(1).strip()
            continue
        if not in_comment:
          comment = []
          reset_comment_after_use = not previous_line_was_blank
        comment.append(line[1:].strip())
        in_comment = True
        continue
      in_comment = False

      if line == '':
        comment = []
        previous_line_was_blank = True
        continue

      # If the line ends with a comment, strip the comment and store it for this
      # line only.
      line, _, line_comment = line.partition('#')
      line = line.strip()
      line_comment = [line_comment.strip()] if line_comment else []

      previous_line_was_blank = False
      if line == 'set noparent':
        self._stop_looking.setdefault(
          self._get_root_affected_dir(dirpath), set()).add(dirpath)
        continue

      m = re.match('per-file (.+)=(.+)', line)
      if m:
        glob_string = m.group(1).strip()
        directive = m.group(2).strip()
        full_glob_string = self.os_path.join(self.root, dirpath, glob_string)
        if '/' in glob_string or '\\' in glob_string:
          raise SyntaxErrorInOwnersFile(owners_path, lineno,
              'per-file globs cannot span directories or use escapes: "%s"' %
              line)
        relative_glob_string = self.os_path.relpath(full_glob_string, self.root)
        self._add_entry(relative_glob_string, directive, owners_path,
                        lineno, '\n'.join(comment + line_comment))
        if reset_comment_after_use:
          comment = []
        continue

      if line.startswith('set '):
        raise SyntaxErrorInOwnersFile(owners_path, lineno,
            'unknown option: "%s"' % line[4:].strip())

      self._add_entry(dirpath, line, owners_path, lineno,
                      ' '.join(comment + line_comment))
      if reset_comment_after_use:
        comment = []

  def _read_global_comments(self):
    if not self._status_file:
      if not 'OWNERS' in self.read_files:
        self._read_owners('OWNERS')
      if not self._status_file:
        return

    owners_status_path = self.os_path.join(self.root, self._status_file)
    if not self.os_path.exists(owners_status_path):
      raise IOError('Could not find global status file "%s"' %
                    owners_status_path)

    if owners_status_path in self.read_files:
      return

    self.read_files.add(owners_status_path)

    lineno = 0
    for line in self.fopen(owners_status_path):
      lineno += 1
      line = line.strip()
      if line.startswith('#'):
        continue
      if line == '':
        continue

      m = re.match('(.+?):(.+)', line)
      if m:
        owner = m.group(1).strip()
        comment = m.group(2).strip()
        if not self.email_regexp.match(owner):
          raise SyntaxErrorInOwnersFile(owners_status_path, lineno,
              'invalid email address: "%s"' % owner)

        self.comments.setdefault(owner, {})
        self.comments[owner][GLOBAL_STATUS] = comment
        continue

      raise SyntaxErrorInOwnersFile(owners_status_path, lineno,
          'cannot parse status entry: "%s"' % line.strip())

  def _add_entry(self, owned_paths, directive, owners_path, lineno, comment):
    # Consistently uses paths with slashes as the keys or else Windows will
    # break in surprising and untested ways.
    owned_paths = owned_paths.replace(os.sep, '/')
    if directive == 'set noparent':
      self._stop_looking.setdefault(
        self._get_root_affected_dir(owned_paths), set()).add(owned_paths)
    elif directive.startswith('file:'):
      include_file = self._resolve_include(directive[5:], owners_path, lineno)
      if not include_file:
        raise SyntaxErrorInOwnersFile(owners_path, lineno,
            ('%s does not refer to an existing file.' % directive[5:]))

      included_owners = self._read_just_the_owners(include_file)
      for owner in included_owners:
        self._owners_to_paths.setdefault(owner, set()).add(owned_paths)
        self._paths_to_owners.setdefault(
          self._get_root_affected_dir(owned_paths), {}).setdefault(
            owned_paths, set()).add(owner)
    elif self.email_regexp.match(directive) or directive == EVERYONE:
      if comment:
        self.comments.setdefault(directive, {})
        self.comments[directive][owned_paths] = comment
      self._owners_to_paths.setdefault(directive, set()).add(owned_paths)
      self._paths_to_owners.setdefault(
        self._get_root_affected_dir(owned_paths), {}).setdefault(
          owned_paths, set()).add(directive)
    else:
      raise SyntaxErrorInOwnersFile(owners_path, lineno,
          ('"%s" is not a "set noparent", file include, "*", '
           'or an email address.' % (directive,)))

  def _resolve_include(self, path, start, lineno):
    if path.startswith('//'):
      include_path = path[2:]
    else:
      assert start.startswith(self.root)
      start = self.os_path.dirname(self.os_path.relpath(start, self.root))
      include_path = self.os_path.normpath(self.os_path.join(start, path))

    if include_path in self.override_files:
      return include_path

    owners_path = self.os_path.join(self.root, include_path)
    # Paths included via "file:" must end in OWNERS or _OWNERS. Files that can
    # affect ownership have a different set of ownership rules, so that users
    # cannot self-approve changes adding themselves to an OWNERS file.
    if not self._file_affects_ownership(owners_path):
      raise SyntaxErrorInOwnersFile(start, lineno, 'file: include must specify '
                                    'a file named OWNERS or ending in _OWNERS')

    if not self.os_path.exists(owners_path):
      return None

    return include_path

  def _read_just_the_owners(self, include_file):
    if include_file in self._included_files:
      return self._included_files[include_file]

    owners = set()
    self._included_files[include_file] = owners
    lineno = 0
    if include_file in self.override_files:
      file_iter = self.override_files[include_file]
    else:
      file_iter = self.fopen(self.os_path.join(self.root, include_file))
    for line in file_iter:
      lineno += 1
      line = line.strip()
      if (line.startswith('#') or line == '' or
              line.startswith('set noparent') or
              line.startswith('per-file')):
        continue

      # If the line ends with a comment, strip the comment.
      line, _delim, _comment = line.partition('#')
      line = line.strip()

      if self.email_regexp.match(line) or line == EVERYONE:
        owners.add(line)
        continue
      if line.startswith('file:'):
        sub_include_file = self._resolve_include(line[5:], include_file, lineno)
        sub_owners = self._read_just_the_owners(sub_include_file)
        owners.update(sub_owners)
        continue

      raise SyntaxErrorInOwnersFile(include_file, lineno,
          ('"%s" is not a "set noparent", file include, "*", '
           'or an email address.' % (line,)))
    return owners

  def _covering_set_of_owners_for(self, files, author):
    dirs_remaining = set()
    for f in files:
      dir_path = self.enclosing_dir_with_owners(f)
      # Always use slashes as separators.
      dirs_remaining.add(dir_path.replace(os.sep, '/'))

    all_possible_owners = self.all_possible_owners(dirs_remaining, author)
    suggested_owners = set()
    while dirs_remaining and all_possible_owners:
      owner = self.lowest_cost_owner(all_possible_owners, dirs_remaining)
      suggested_owners.add(owner)
      dirs_to_remove = set(el[0] for el in all_possible_owners[owner])
      dirs_remaining -= dirs_to_remove
      # Now that we've used `owner` and covered all their dirs, remove them
      # from consideration.
      del all_possible_owners[owner]
      for o, dirs in list(all_possible_owners.items()):
        new_dirs = [(d, dist) for (d, dist) in dirs if d not in dirs_to_remove]
        if not new_dirs:
          del all_possible_owners[o]
        else:
          all_possible_owners[o] = new_dirs
    return suggested_owners

  def _all_possible_owners_for_dir_or_file(self, dir_or_file, author,
                                           cache):
    """Returns a dict of {potential owner: (dir_or_file, distance)} mappings.
    """
    assert not dir_or_file.startswith("/")
    res = cache.get(dir_or_file)
    if res is None:
      res = {}
      dirname = dir_or_file
      for owner in self._owners_for(dirname):
        if author and owner == author:
          continue
        res.setdefault(owner, [])
        res[owner] = (dir_or_file, 1)
      if not self._should_stop_looking(dirname):
        dirname = self.os_path.dirname(dirname)

        parent_res = self._all_possible_owners_for_dir_or_file(dirname,
                                                               author, cache)

        # Merge the parent information with our information, adjusting
        # distances as necessary, and replacing the parent directory
        # names with our names.
        for owner, par_dir_and_distances in parent_res.items():
          if owner in res:
            # If the same person is in multiple OWNERS files above a given
            # directory, only count the closest one.
            continue
          parent_distance = par_dir_and_distances[1]
          res[owner] = (dir_or_file, parent_distance + 1)

      cache[dir_or_file] = res

    return res

  def all_possible_owners(self, dirs_and_files, author):
    """Returns a dict of {potential owner: (dir, distance)} mappings.

    A distance of 1 is the lowest/closest possible distance (which makes the
    subsequent math easier).
    """

    self.load_data_needed_for(dirs_and_files)
    all_possible_owners_for_dir_or_file_cache = {}
    all_possible_owners = {}
    for current_dir in dirs_and_files:
      # Always use slashes as separators.
      current_dir = current_dir.replace(os.sep, '/')
      dir_owners = self._all_possible_owners_for_dir_or_file(
        current_dir, author,
        all_possible_owners_for_dir_or_file_cache)
      for owner, dir_and_distance in dir_owners.items():
          if owner in all_possible_owners:
            all_possible_owners[owner].append(dir_and_distance)
          else:
            all_possible_owners[owner] = [dir_and_distance]

    return all_possible_owners

  def _fnmatch(self, filename, pattern):
    """Same as fnmatch.fnmatch(), but internally caches the compiled regexes."""
    # Make sure backslashes are never used in the filename. The regex
    # expressions generated by fnmatch.translate don't handle matching slashes
    # to backslashes.
    filename = filename.replace(os.sep, '/')
    assert pattern.count('\\') == 0, 'Backslashes found in %s' % pattern
    matcher = self._fnmatch_cache.get(pattern)
    if matcher is None:
      matcher = re.compile(fnmatch.translate(pattern)).match
      self._fnmatch_cache[pattern] = matcher
    return matcher(filename)

  @staticmethod
  def total_costs_by_owner(all_possible_owners, dirs):
    # We want to minimize both the number of reviewers and the distance
    # from the files/dirs needing reviews. The "pow(X, 1.75)" below is
    # an arbitrarily-selected scaling factor that seems to work well - it
    # will select one reviewer in the parent directory over three reviewers
    # in subdirs, but not one reviewer over just two.
    result = {}
    for owner in all_possible_owners:
      total_distance = 0
      num_directories_owned = 0
      for dirname, distance in all_possible_owners[owner]:
        if dirname in dirs:
          total_distance += distance
          num_directories_owned += 1
      if num_directories_owned:
        result[owner] = (total_distance /
                         pow(num_directories_owned, 1.75))
    return result

  @staticmethod
  def lowest_cost_owner(all_possible_owners, dirs):
    total_costs_by_owner = Database.total_costs_by_owner(all_possible_owners,
                                                         dirs)
    # Return the lowest cost owner. In the case of a tie, pick one randomly.
    lowest_cost = min(total_costs_by_owner.values())
    lowest_cost_owners = [
        owner for owner, cost in total_costs_by_owner.items()
        if cost == lowest_cost]
    return random.Random().choice(lowest_cost_owners)

  def owners_rooted_at_file(self, filename):
    """Returns a set of all owners transitively listed in filename.

    This function returns a set of all the owners either listed in filename, or
    in a file transitively included by filename. Lines that are not plain owners
    (i.e. per-file owners) are ignored.
    """
    return self._read_just_the_owners(filename)
