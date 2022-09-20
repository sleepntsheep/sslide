# sslide

[sent](https://tools.suckless.org/sent/) inspired presentation tool.

This tool was created because I love sent,
but it's a bit too plain (no multiple image, image+text)
and it only support Unix (more specifically, unix running X11)

### Features
- Multiple **frames** per page
- Custom frame location and size
- Auto content resizing
- Image support
- custom font by creating $HOME/.sslide.json ($USERPROFILE/.sslide.json on windows)
- Cross platform (SDL2 and [tinyfiledialog](http://tinyfiledialogs.sourceforge.net/))

### Non-Features
- pdf exporting - you can set up a script press k and screenshot

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
```

#### Visual Studio

Not supported

