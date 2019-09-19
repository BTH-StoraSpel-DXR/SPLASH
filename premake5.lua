-- premake5.lua
workspace "Sail"
	configurations { "Debug", "Release" }
	startproject "SPLASH"
	platforms { "DX11 x64", "DX11 x86",
				"DX12 x64", "DX12 x86"
				-- "Vulkan x64", "Vulkan x86",
			  }

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

group "Libraries"
include "libraries/glfw"
include "libraries/imgui"

group ""
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
		"%{prj.name}/resource.h", -- For icon
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp"
	}

	-- include and fix these as soon as new cross-platform architecture is finished
	removefiles { 
		"**/ParticleHandler.*",
		"**/PlayerCameraController.*",
		"**/Scene.*"
	}

	includedirs {
		"libraries",
		"Sail/src",
		"%{IncludeDir.FBX_SDK}",
		"%{IncludeDir.ImGui}",
		"Physics"
	}

	links {
		"Sail",
		"Physics"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"

	-- Copy fbxsdk dll to executable path
	filter { "action:vs2017 or vs2019", "platforms:*64" }
		postbuildcommands {
			"{COPY} \"../libraries/FBX_SDK/lib/vs2017/x64/%{cfg.buildcfg}/libfbxsdk.dll\" \"%{cfg.targetdir}\""
		}
	filter { "action:vs2017 or vs2019", "platforms:*86" }
		postbuildcommands {
			"{COPY} \"../libraries/FBX_SDK/lib/vs2017/x86/%{cfg.buildcfg}/libfbxsdk.dll\" \"%{cfg.targetdir}\""
		}


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
		"%{prj.name}/src/API/DX11/**",
		"%{prj.name}/src/API/DX12/**",
		"%{prj.name}/src/API/VULKAN/**",

		"**/DXCubeMap.*",
		"%{prj.name}/src/Sail/resources/audio/**",
		"**/Skybox.*",
		"**/ParticleEmitter.*",
		"%{prj.name}/src/Sail/graphics/shadows/**",
		"%{prj.name}/src/Sail/graphics/shader/instanced/**",
		"%{prj.name}/src/Sail/graphics/shader/deferred/**",
		"%{prj.name}/src/Sail/graphics/shader/component/ConstantBuffer**",
		"%{prj.name}/src/Sail/graphics/shader/component/Sampler**",
		"%{prj.name}/src/Sail/graphics/shader/basic/**",
		"%{prj.name}/src/Sail/graphics/renderer/**",
		"**/Quadtree.*"
	}

	includedirs {
		"libraries",
		"Sail/src",
		"%{IncludeDir.FBX_SDK}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.ImGui}"
	}

	links {
		"libfbxsdk",
		"GLFW",
		"ImGui"
	}

	defines {
		"SAIL_PLATFORM=\"%{cfg.platform}\""
	}

	filter { "platforms:DX11*" }
		defines {
			"_SAIL_DX11"
		}
		files {
			"%{prj.name}/src/API/DX11/**",
			"%{prj.name}/src/API/Windows/**"
		}
	filter { "platforms:DX12*" }
		defines {
			"_SAIL_DX12"
		}
		files {
			"%{prj.name}/src/API/DX12/**",
			"%{prj.name}/src/API/Windows/**"
		}

	filter { "action:vs2017 or vs2019", "platforms:*64" }
		libdirs {
			"libraries/FBX_SDK/lib/vs2017/x64/%{cfg.buildcfg}"
		}
	filter { "action:vs2017 or vs2019", "platforms:*86" }
		libdirs {
			"libraries/FBX_SDK/lib/vs2017/x86/%{cfg.buildcfg}"
		}

	filter "system:windows"
		systemversion "latest"

		defines {
			"FBXSDK_SHARED",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"

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
	
	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"
