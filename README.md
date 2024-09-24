# DEdit

This is my personal text editor i use it to build pretty much everything.
editor is tailored to my personal needs, so it's needless to sat that MOST of the features are opinionated

Editor is primarily meant to be used with C and C++, it has built in support for clang-format no LSP or any autocomplete tools.

### Some of the shortcuts

Ctr+P - Command Palette

Ctr+B - Trigger build command

Ctr+D - Duplicate line

Ctr+A - Select whole buffer

Ctr+C - Copy from buffer

Ctr+V - Paste to buffer

Ctr+X - Cut from buffer

Ctr+UP - Scroll view up with respect of cursor 

Ctr+DOWN - Scroll view down with respect of cursor 

Shift+Fn+[ - select line from cursor position to line end

Shift+Fn+] - select line from cursor position to line start

Ctr+M - Jump to middle of the line

Fn+[ - Jump to start of the line

Fn+] = Jump to end of the line


to use build command you will need to create project_config.json file that will have following command

```json
{
  "build_command" : "shell command"
}
```
