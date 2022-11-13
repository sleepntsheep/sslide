# sslide

[sent](https://tools.suckless.org/sent/) inspired presentation tool.

### Why

This tool was created because I love sent,
but it's a bit too plain (no multiple image, image+text)
and it only support Unix (more specifically, unix running X11)

### Features

- Multiple **frames** per page
- Custom frame location and size
- Auto content resizing
- Auto detection for best font for rendering each frame
- Image support
- Invert color by pressing **i**
- Per-slide config system (see example/slide)
- Cross platform (SDL2 and [tinyfiledialog](http://tinyfiledialogs.sourceforge.net/))

### Configuration

Put a section starting with `---` and ending with `---` at the start
of your slide file, then specifies these properties.
All of them are optional and has default values as following.

    ---
    bg = FFFFFF
    fg = 000000
    #font =
    linespacing = 3
    progressbarheight = 5
    simple = false
    marginy = 5
    marginx = 10
    ---

    | Properties        |                  Format                      |
    | bg                | string - 6 digit hex code without leading 0x |
    | fg                | string - 6 digit hex code without leading 0x |
    | font              | string - absolute/relative path to ttf font  |
    | linespacing       | integer - spacing between each line          |
    | progressbarheight | integer - height of progress bar (can be 0)  |
    | simple            | bool - true or false (simple mode)           |
    | marginy           | integer - vertical margin in percent         |
    | marginx           | integer - horizontal margin in percent       |


##### Simple mode

This turn off multiple frames per page feature, hence also discarding the need for extra
frame geometry and page terminator syntax, so you can use that on plain text
document and it will work. But this also work with normal sslide document!

### Non-Features

- pdf exporting: you can set up a script press k and screenshot
- macros: use find-and-replace in your text editor!

### Building instructions

#### MSYS2 on Windows:

Install libsdl2, premake from msys' repository

```
premake5 gmake2
mingw32-make config=mingw
```

#### Linux

Install libsdl2, premake from your distro's repository

```
premake5 gmake2
make config=release
sudo install -Dm755 Release/sslide /usr/local/bin
sudo desktop-file-install sslide.desktop
```

#### Visual Studio

Not supported

