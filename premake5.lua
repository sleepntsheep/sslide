-- premake5.lua
workspace "sslide"
   configurations { "Debug", "Release", "Mingw" }

project "sslide"
   kind "ConsoleApp"
   language "C"
   targetdir "%{cfg.buildcfg}"

   files { "*.h", "*.c" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

   filter "configurations:Mingw"
      system "Windows"
      defines { "main=SDL_main" }
      links { "SDL2main", "mingw32" }

   links { "SDL2", "SDL2_ttf", "SDL2_image" }
