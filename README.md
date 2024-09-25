<div align="center">
  <img src="https://raw.githubusercontent.com/dkvilo/dk_edit/refs/heads/master/res/icon.png" width="200" />
</div>
<br />


This is my personal text editor, which I use to build pretty much everything. The editor is tailored to my personal needs, so it's needless to say that **most** of the features are opinionated.

The editor is primarily meant for use with C and C++. It has built-in support for `clang-format`, but no LSP or autocomplete tools.

This version of editor renderer is implemented on top of `WGPU`, there is also `OpenGL` version. 

![Screenshot](/screenshots/s0.png)

![Screenshot](/screenshots/s1.png)

![Screenshot](/screenshots/s2.png)

![Screenshot](/screenshots/s3.png)


### Some of the shortcuts

| **Shortcut**        | **Action**                                       |
|---------------------|--------------------------------------------------|
| `Ctrl + P`          | Command Palette                                  |
| `Ctrl + B`          | Trigger build command                            |
| `Ctrl + D`          | Duplicate line                                   |
| `Ctrl + A`          | Select whole buffer                              |
| `Ctrl + C`          | Copy from buffer                                 |
| `Ctrl + V`          | Paste to buffer                                  |
| `Ctrl + X`          | Cut from buffer                                  |
| `Ctrl + ↑`          | Scroll view up with respect to cursor            |
| `Ctrl + ↓`          | Scroll view down with respect to cursor          |
| `Shift + Fn + [`    | Select from cursor to line end                   |
| `Shift + Fn + ]`    | Select from cursor to line start                 |
| `Ctrl + M`          | Jump to the middle of the line                   |
| `Fn + [`            | Jump to the start of the line                    |
| `Fn + ]`            | Jump to the end of the line                      |


## Command Palette

'@' symbols in current buffer

'#' tasks (todo, note) in the current buffer

'/' system command


to use build command you will need to create project_config.json file that will have following command

```json
{
  "build_command" : "shell command"
}
```

other configuration options

```json
{
  "build_command" : "make",
  "format_on_save": true,
  "formatter": {
    "bin": "clang-format", // here you can specify clang format absolute path, if needed
    "style": "Mozilla" // Google, LLVM and etc
  }
}
```
