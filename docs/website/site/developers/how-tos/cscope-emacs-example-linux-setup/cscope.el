(load "/usr/share/emacs/site-lisp/xcscope.el")

(setq cscope-do-not-update-database t)
(setq cscope-initial-directory nil)

(defun ami-src-root-for-buffer ()
  "Return the source root for the current buffer"
  (cond
   ((null buffer-file-name) (error "Unexpected no path!"))
   ((string-match "/chromium/src/" buffer-file-name) "~/src/chromium/src")
   ((string-match "/wr/trunk/" buffer-file-name) "~/src/wr/trunk")
   (t (error (concat "Unexpected path: " buffer-file-name)))))
(defun ami-cscope-symbol-feeling-lucky (sym)
  "Do a cscope symbol lookup trying for the declaration of a type."
  (interactive (list
                (cscope-prompt-for-symbol "Find this symbol: " nil)
                ))
  (let* ((file-line
          (shell-command-to-string
           (format (concat "cd %s/.. && "
                           "cscope -dL -1 %s |egrep '[0-9] *((struct|class|#define)( [A-Z]*_EXPORT)? %s($| |\\()|typedef .* %s(;?| {)$)'|" ;; #
                           "head -1 |cut -d' ' -f1,3|tr ' ' ':'")
                   (ami-src-root-for-buffer) sym sym sym))))
    (if (not (string= "" file-line))
	(let* ((parts (split-string file-line ":"))
	       (file (car parts))
	       (line (string-to-int (cadr parts))))
	  (find-file file)
	  (goto-line line))
      (cscope-find-this-symbol sym))))
(defun ami-cscope-at-point ()
  "Invoke reasonable cscope command depending on the context.
Useful in chromium."
  (interactive)
  (let ((line (buffer-substring-no-properties
               (line-beginning-position) (line-end-position))))
    (if (string-match "^#include [\"<]" line)
        (call-interactively 'cscope-find-this-file)
      (call-interactively 'ami-cscope-symbol-feeling-lucky))))

(global-set-key '[?\M-.] 'ami-cscope-at-point)
