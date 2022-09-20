-- premake5.lua
workspace "sslide"
   configurations { "Debug", "Release" }

project "sslide"
   kind "ConsoleApp"
   language "C"
   targetdir "%{cfg.buildcfg}"

   files { "*.h", "*.c" }

   links { "SDL2", "SDL2_ttf", "SDL2_image" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
