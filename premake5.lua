-- premake5.lua
workspace "sslide"
configurations { "Debug", "Release", "Mingw" }

newaction {
    trigger     = "clean",
    description = "clean the build directory",
    execute     = function ()
        print("----\nCleaning")
        os.rmdir("./build")
        print("Removed ./build")
        os.rmdir("Debug")
        print("Removed ./Debug")
        os.rmdir("Release")
        print("Removed ./Release")
        os.rmdir("Mingw")
        print("Removed ./Mingw")
        os.rmdir("Obj")
        print("Removed ./Obj")
        print("Cleaning done.\n------")
    end
}

project "sslide"
kind "WindowedApp"
language "C"
targetdir "%{cfg.buildcfg}"

files { "src/**.h", "src/**.c" }

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

filter {}
buildoptions { "-std=c99", "-Wall", "-Wall", "-Wextra", "-pedantic" }
links { "SDL2", "SDL2_ttf", "SDL2_image", "fontconfig" }

