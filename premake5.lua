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

project "sslide"
kind "ConsoleApp"
language "C"
targetdir "%{cfg.buildcfg}"

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
    defines { "USE_UNIFONT" }

filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"
    defines { "USE_UNIFONT" }

filter {}
    links { "SDL2", "SDL2_ttf", "SDL2_image" }

filter "configurations:MinimalFont"
    defines { "USE_UNIFONT=0" }


