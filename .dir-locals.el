((nil
  . ((indent-tabs-mode . nil)
     (show-trailing-whitespace . t)))
 (c-mode
  . ((mode . c++)))
 (c++-mode
  . ((eval add-hook 'before-save-hook #'clang-format-buffer nil t)))
 (nix-mode
  . ((mode . nixfmt-on-save)))
 (cmake-mode
  . ((eval add-hook 'before-save-hook #'cmake-format-buffer nil t))))
