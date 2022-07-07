-- premake5.lua
workspace "Raytracer"
   location "build"
   configurations { "Debug", "Release" }

project "Raytracer"
   location "build/Raytracer"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++17"
   targetdir "bin/%{cfg.buildcfg}"
   
   sysincludedirs {"/usr/local/include", "ThirdParty/MoltenVK/include", "ThirdParty/stb"}
   includedirs {""}

   files { "Base/**.h", "Base/**.cpp", "Raytracer/**.h", "Raytracer/**.cpp", "*.lua", "**.md" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
      
   configuration "macosx"
      libdirs {"/usr/local/lib"}
      links {"CoreFoundation.framework", "Cocoa.framework", "IOKit.framework", "glfw", "vulkan"}

   configuration {"macosx", "gmake"}
      buildoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
      linkoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}


