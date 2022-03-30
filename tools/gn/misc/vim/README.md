# GN vim syntax plugin

## Installation with a plugin manager

You can use modern plugin managers to download the GN repo and manage the vim
plugin:

Example config for [vim-plug](https://github.com/junegunn/vim-plug):

```
Plug 'https://gn.googlesource.com/gn', { 'rtp': 'misc/vim' }
```

Or, for [Vundle](https://github.com/VundleVim/Vundle.vim) users:

```
Plugin 'https://gn.googlesource.com/gn', { 'rtp': 'misc/vim' }
```

## Manual installation

If you don't use a plugin manager or would prefer to manage the GN repo
yourself, you can add this explicitly to `rtp` in your `.vimrc`:

```
set runtimepath+=/path/to/gn/misc/vim
" ...
filetype plugin indent on " or a similar command to turn on filetypes in vim
```

## Formatting GN files

### vim-codefmt (recommended)

[vim-codefmt](https://github.com/google/vim-codefmt) supports the GN filetype
natively. Add the following to your `.vimrc`:

```vim
" Install vim-codefmt and its dependencies
Plug 'google/vim-maktaba'
Plug 'google/vim-codefmt'

" Install this plugin:
Plug 'https://gn.googlesource.com/gn', { 'rtp': 'misc/vim' }

" Optional: configure vim-codefmt to autoformat upon saving the buffer.
augroup CodeFmt
  autocmd!
  autocmd FileType gn AutoFormatBuffer gn
  " Other file types...
augroup END
```

This will autoformat your files every time you save. If you prefer not to format
files upon saving, vim-codefmt can format the buffer by calling `:FormatCode`.

### Included format integration

If you cannot include vim-codefmt, you can use the limited `gn format`
integration included in this plugin. Add the following to your `.vimrc`:

```vim
" Replace <F1> with whichever hotkey you prefer:
nnoremap <silent> <F1> :pyxf <path-to-this-plugin>/gn-format.py<CR>
```
