-- premake5.lua
workspace "sslide"
configurations { "Debug", "Release", "Mingw" }

newaction {
    trigger     = "clean",
    description = "clean",
    execute     = function ()
        print("----\nCleaning")
        os.rmdir("./build")
        os.rmdir("Debug")
        os.rmdir("Release")
        os.rmdir("Mingw")
        os.rmdir("obj")
        os.remove("sslide.make")
        os.remove("Makefile")
        print("Cleaning done.\n------")
    end
}

project "sslide"
kind "WindowedApp"
language "C"
targetdir "%{cfg.buildcfg}"
cdialect "c99"

files { "src/**.h", "src/**.c" }

filter "configurations:Mingw"
system "Windows"
defines { "NDEBUG" }
optimize "On"
defines { "main=SDL_main" }
links { "mingw32", "SDL2main", "comdlg32", "ole32", "shlwapi" }

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"

filter "configurations:Release"
defines { "NDEBUG" }
optimize "On"

filter {}
buildoptions { "-Wall", "-Wall", "-Wextra", "-pedantic" }
links { "SDL2", "SDL2_ttf", "SDL2_image", "fontconfig" }

