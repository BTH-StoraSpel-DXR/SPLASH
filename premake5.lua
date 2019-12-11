-- premake5.lua
workspace "Sail"
	configurations { "Debug", "PerformanceTest", "Dev-Release", "Release"  }
	startproject "SPLASH"
	platforms { "DX12 x64", "DX12 x86" }

	filter "platforms:*86"
		architecture "x86"

	filter "platforms:*64"
		architecture "x64"

local binDir = "bin/%{cfg.platform}-%{cfg.buildcfg}/%{prj.name}"
local intermediatesDir = "intermediates/%{prj.name}-%{cfg.platform}-%{cfg.buildcfg}"

IncludeDir = {}
IncludeDir["GLFW"] = "libraries/glfw/include"
IncludeDir["FBX_SDK"] = "libraries/FBX_SDK/include"
IncludeDir["ImGui"] = "libraries/imgui"
IncludeDir["MiniMM"] = "libraries/MemoryManager"
IncludeDir["Assimp"] = "libraries/assimp/include"

buildcfg = "release"

group "Libraries"
include "libraries/glfw"
include "libraries/imgui"
include "libraries/MemoryManager"

group ""

-----------------------------------
-------------  SPLASH -------------
-----------------------------------
project "SPLASH"
	location "SPLASH"
	kind "WindowedApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir (binDir)
	objdir (intermediatesDir)

	files {
		"%{prj.name}/SPLASH.rc",    -- For icon
		"%{prj.name}/resource.h", 	-- For icon
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp"
	}

	includedirs {
		"libraries",
		"Sail/src",
		"%{IncludeDir.ImGui}",
		"Physics"
	}

	links {
		"Sail",
		"Physics"
	}

	defines { "NOMINMAX",				-- Removes min max macros which cause issues
			  "WIN32_LEAN_AND_MEAN" }	-- Exclude some less used APIs to speed up the build process on windows

	flags { "MultiProcessorCompile" }

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines { "DEBUG", "DEVELOPMENT" }
		symbols "On"
		buildCfg = "debug"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"

	filter "configurations:PerformanceTest"
		defines { "NDEBUG", "_PERFORMANCE_TEST", "DEVELOPMENT" }
		optimize "On"

	filter "configurations:Dev-Release"
		defines { "NDEBUG", "DEVELOPMENT" }
		optimize "On"

	-- Copy dlls to executable path
	filter { "action:vs2017 or vs2019", "platforms:*64" }
		postbuildcommands {

		}
	filter { "action:vs2017 or vs2019", "platforms:*86" }
		postbuildcommands {

		}


-----------------------------------
--------------  Sail --------------
-----------------------------------
project "Sail"
	location "Sail"
	kind "StaticLib"
	language "C++"
	targetdir "bin/%{cfg.platform}-%{cfg.buildcfg}"
	objdir (intermediatesDir)
	cppdialect "C++17"
	staticruntime "on"

	pchheader "pch.h"
	pchsource "Sail/src/pch.cpp"

	files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp"
	}

	-- include and fix these as soon as new cross-platform architecture is finished
	removefiles {
		"%{prj.name}/src/API/DX12/**",
		"%{prj.name}/src/API/VULKAN/**",
	}

	includedirs {
		"libraries",
		"Sail/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.MiniMM}",
	}

	links {
		"GLFW",
		"ImGui",
		"MemoryManager",
		"WinMMx"
	}

	flags { "MultiProcessorCompile" }

	defines {
		"SAIL_PLATFORM=\"%{cfg.platform}\""
	}

	filter { "platforms:DX12*" }
		defines {
			"_SAIL_DX12"
		}
		files {
			"%{prj.name}/src/API/DX12/**",
			"%{prj.name}/src/API/Windows/**"
		}

	filter "configurations:Debug"
		defines { "DEBUG", "DEVELOPMENT" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"

	filter "configurations:PerformanceTest"
		defines { "NDEBUG", "_PERFORMANCE_TEST", "DEVELOPMENT" }
		optimize "On"

	filter "configurations:Dev-Release"
		defines { "NDEBUG", "DEVELOPMENT" }
		optimize "On"

	filter { "action:vs2017 or vs2019", "platforms:*64" }
		libdirs {
			"libraries/WinMM/x64"
		}
	filter { "action:vs2017 or vs2019", "platforms:*86" }
		libdirs {
			"libraries/WinMM/x86"
		}

	filter "system:windows"
		systemversion "latest"

		defines {
			"FBXSDK_SHARED",
			"GLFW_INCLUDE_NONE"
		}

-----------------------------------
-------------  Physics ------------
-----------------------------------
project "Physics"
	location "Physics"
	kind "StaticLib"
	language "C++"
	targetdir "bin/%{cfg.platform}-%{cfg.buildcfg}"
	objdir (intermediatesDir)
	cppdialect "C++17"
	staticruntime "on"

	pchheader "PhysicsPCH.h"
	pchsource "Physics/PhysicsPCH.cpp"

	files {
		"%{prj.name}/**.h",
		"%{prj.name}/**.cpp"
	}

	includedirs {
		"libraries",
		"Sail/src"
	}

	links {
		"Sail"
	}

	flags { "MultiProcessorCompile" }

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines { "DEBUG", "DEVELOPMENT" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"

	filter "configurations:PerformanceTest"
		defines { "NDEBUG", "_PERFORMANCE_TEST", "DEVELOPMENT" }
		optimize "On"
		
	filter "configurations:Dev-Release"
		defines { "NDEBUG", "DEVELOPMENT" }
		optimize "On"

-----------------------------------
------- NOT FBX CONVERTER ---------
-----------------------------------
project "FBXToNotFBXConverter"
	location "FBXToNotFBXConverter"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir (binDir)
	objdir (intermediatesDir)

	files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/../Sail/src/Sail/resources/loaders/NotFBXLoader.cpp",
		"%{prj.name}/../Sail/src/Sail/resources/loaders/NotFBXLoader.h",
	}   

	includedirs {
		"libraries",
		"%{prj.name}/src/",
		"%{IncludeDir.FBX_SDK}",
		"%{prj.name}/../Sail/src/Sail/resources/loaders/",
	}

	links {
		"libfbxsdk",
	}

	defines { "NOMINMAX",				-- Removes min max macros which cause issues
			  "WIN32_LEAN_AND_MEAN" }	-- Exclude some less used APIs to speed up the build process on windows

	flags { "MultiProcessorCompile" }
	
	filter "system:windows"
		systemversion "latest"
		
		defines {
			"FBXSDK_SHARED",
			"FBX_CONVERTER"
		}

	filter "configurations:Debug"
		defines { "DEBUG", "DEVELOPMENT" }
		symbols "On"
		buildCfg = "debug"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"

	filter "configurations:Dev-Release"
		defines { "NDEBUG", "DEVELOPMENT" }
		optimize "On"

	-- Copy dlls to executable path
	filter { "action:vs2017 or vs2019", "platforms:*64" }
		libdirs {
			"libraries/FBX_SDK/lib/vs2017/x64/%{buildCfg}",
		}
		postbuildcommands {
			"{COPY} \"../libraries/FBX_SDK/lib/vs2017/x64/%{buildCfg}/libfbxsdk.dll\" \"%{cfg.targetdir}\"",
		}
		
	filter { "action:vs2017 or vs2019", "platforms:*86" }
		libdirs {
			"libraries/FBX_SDK/lib/vs2017/x86/%{buildCfg}",
		}
		postbuildcommands {
			"{COPY} \"../libraries/FBX_SDK/lib/vs2017/x86/%{buildCfg}/libfbxsdk.dll\" \"%{cfg.targetdir}\"",
		}
	