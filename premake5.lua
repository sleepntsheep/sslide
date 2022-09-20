-- premake5.lua
workspace "sslide"
   configurations { "Debug", "Release", "Mingw" }

newaction {
   trigger     = "clean",
   description = "clean the software",
   execute     = function ()
      print("clean the build...")
      os.rmdir("./build")
      os.rmdir("Debug")
      os.rmdir("Release")
      os.rmdir("Mingw")
      os.rmdir("Obj")
      print("done.")
   end
}

project "sslide"
   kind "ConsoleApp"
   language "C"
   targetdir "%{cfg.buildcfg}"

   files { "*.h", "*.c" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"
      links { "SDL2", "SDL2_ttf", "SDL2_image" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
      links { "SDL2", "SDL2_ttf", "SDL2_image" }

   filter "configurations:Mingw"
      system "Windows"
      defines { "NDEBUG" }
      optimize "On"
      defines { "main=SDL_main" }
      links { "mingw32", "SDL2main", "SDL2", "SDL2_ttf", "SDL2_image", "comdlg32", "ole32" }

      
