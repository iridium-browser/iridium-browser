---
breadcrumbs:
- - /developers
  - For Developers
page_name: checkstyle
title: Checkstyle
---

> *Checkstyle is a development tool to help programmers write Java code that
> adheres to a coding standard. It automates the process of checking Java code
> to spare humans of this boring (but important) task. This makes it ideal for
> projects that want to enforce a coding standard.
> (*<http://checkstyle.sourceforge.net/>)

**Eclipse**

1.  (Package Explorer) Right click and click 'Refresh'
2.  Help-&gt;Install New Software...
3.  Work with -&gt; Add: http://eclipse-cs.sourceforge.net/update
4.  Type 'Checkstyle' into the search box
5.  Select 'Checkstyle' and click-through next
6.  (Menu bar) Window-&gt;Preferences-&gt;Checkstyle-&gt;New
7.  Under 'Type' select 'Project Relative Configuration'
8.  Under 'Name' enter 'chromium-style-5.0'
9.  Click browse and navigate to
            tools/android/checkstyle/chromium-style-5.0.xml
10. Click OK
11. (Package Explorer) Right click project -&gt; Properties -&gt;
            Checkstyle
12. Under "Simple - use the following check configuration for all files"
            select 'chromium-style-5.0'
13. (Recommended) Under 'Exclude from checking...' Select:
    *   files outside source directories
    *   files not opened in editor

**Eclim**

1.  cd &lt;path&gt;/&lt;to&gt;/&lt;eclimd&gt; (e.g.
            ...../eclipse43/stable/)
2.  find . -name 'checkstyle\*.jar' | grep eclim
3.  The latest version (as of 10/17/2014) is 5.8. If you have an older
            version (e.g. checkstyle-5.6.jar, checkstyle-5.5.jar) follow the
            update steps.

*Update checkstyle jar*

1.  Follow Eclipse steps #2 through #5
2.  find . -name 'checkstyle-5.8.jar'
3.  Copy that jar into the same directory as the old one.
4.  vim ./plugins/org.eclim.jdt_2.3.2/META-INF/MANIFEST.MF (jdt version
            number may be different)
5.  Under 'Bundle-ClassPath' edit the old checkstyle\*.jar version
            number to the new one.
6.  Restart eclimd

*Configure Eclim*

1.  Open vim and enter :EclimSettings
2.  Find the line org.eclim.java.checkstyle.config=
3.  Append
            /your/project/path/src/tools/android/checkstyle/chromium-style-5.0.xml
4.  Open a Java file and enter :Checkstyle to verify it works
5.  (Optional) Add the following to your vimrc:

> > " Run Checkstyle on open/write

> > autocmd BufWinEnter \*.java :Checkstyle

> > autocmd BufWritePost \*.java :Checkstyle

**Emacs**

You can use the built-in flymake to run the checkstyle in the background as you
edit your file. Add this to your ~/.emacs file and change the path to the jar
and conf

;; Check style of Java files.

(require 'flymake)

(add-hook 'find-file-hook 'flymake-find-file-hook)

(defun flymake-java-init ()

(let\* ((temp-file (flymake-init-create-temp-buffer-copy

'flymake-create-temp-inplace))

(local-file (file-relative-name

temp-file

(file-name-directory buffer-file-name))))

(list "java"

(list "-cp"

(expand-file-name "~/.emacs.d/checkstyle-5.9-SNAPSHOT-all.jar")

"com.puppycrawl.tools.checkstyle.Main"

"-c"

(expand-file-name "~/.emacs.d/chromium-style-5.0.xml")

local-file))))

(setq flymake-allowed-file-name-masks

(cons '(".+\\\\.java$"

flymake-java-init

flymake-simple-cleanup

flymake-get-real-file-name)

flymake-allowed-file-name-masks))

(setq flymake-err-line-patterns

(cons '("\\\\(.\*\\\\.java\\\\):\\\\(\[0-9\]+\\\\):\[0-9\]+: \\\\(.+\\\\)" 1 2
nil 3)

flymake-err-line-patterns))

;; Check \*Message\* buffer for errors. If you don't find any, you can remove
this line.

(setq flymake-log-level 3)

Flymake will underline lines with errors and warnings. In GUI mode, Emacs will
show a tooltip when you hover your mouse over the underlined lines. Setup a
shortcut to go to the next error:

;; Go through flymake errors with F4. Show the tooltip with F3. (Tooltips work
only in GUI mode.)

(global-set-key \[f4\] 'flymake-goto-next-error)

(global-set-key \[f3\] 'flymake-display-err-menu-for-current-line)

You can use the built-in C-h . (Control-h period) shortcut to show the error
from the current line in the minibuffer when in running Emacs in a terminal.
Alternatively, you can configure Emacs to show the error in the minibuffer after
a short delay of your cursor being in that line:

;; Display errors in the minibuffer after a short delay.

(setq help-at-pt-display-when-idle t)

(setq help-at-pt-timer-delay 0.2)

(help-at-pt-set-timer)

**Limitations**

Does not support double indentation levels for line-wrap which may generate
spurious warnings.
