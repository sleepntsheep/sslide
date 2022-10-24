-- premake5.lua
workspace "sslide"
configurations { "Debug", "Release", "Mingw", "MinimalFont" }

newaction {
    trigger     = "clean",
    description = "clean the software",
    execute     = function ()
        print("clean the build...")
        os.rmdir("./build")
        os.rmdir("Debug")
        os.rmdir("Release")
        os.rmdir("Mingw")
        os.rmdir("MinimalFont")
        os.rmdir("Obj")
        print("done.")
    end
}

newaction {
    trigger = "install",
    description = "install to path",
    execute = function ()
        os.copyfile("./Release/sslide", "/usr/local/bin")
    end
}

newoption {
    trigger = "sanitize",
    description = "enable sanitizers"
}

project "sslide"
kind "ConsoleApp"
language "C"
targetdir "%{cfg.buildcfg}"

if _OPTIONS["sanitize"] then
    buildoptions { "-fsanitize=address", "-fsanitize=undefined" }
    linkoptions { "-fsanitize=address", "-fsanitize=undefined" }
end

files { "*.h", "*.c" }

filter "configurations:Mingw"
system "Windows"
defines { "NDEBUG" }
optimize "On"
defines { "main=SDL_main" }
links { "mingw32", "SDL2main", "comdlg32", "ole32" }

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"

filter "configurations:Release"
defines { "NDEBUG" }
optimize "On"

filter "configurations:MinimalFont"

filter {}
buildoptions { "-std=c17", "-Wall", "-Wextra", "-pedantic" }
links { "SDL2", "SDL2_ttf", "SDL2_image", "fontconfig" }

