(setq command-switch-alist '(
                             ("-exec"    . cmdline-exec)))

(defun cmdline-exec (name)
  (eval (car (read-from-string (car command-line-args-left))))
)
 