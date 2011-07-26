--
-- Copyright 2010-2011 Branimir Karadzic. All rights reserved.
-- License: http://www.opensource.org/licenses/BSD-2-Clause
--

solution "map2sym"
	configurations {
		"Debug",
		"Release",
	}

	platforms {
		"x32",
		"x64",
	}

	language "C++"

newoption {
	trigger = "gcc",
	value = "GCC",
	description = "Choose GCC flavor",
	allowed = {
		{ "mingw", "MinGW" },
	}
}

-- Avoid error when invoking premake4 --help.
if (_ACTION == nil) then return end

ROOT_DIR = (path.getabsolute("..") .. "/")
BUILD_DIR = (ROOT_DIR .. ".build/")
THIRD_PARTY_DIR = (ROOT_DIR .. "3rdparty/")

location (BUILD_DIR .. "projects/" .. _ACTION)

if _ACTION == "gmake" then

	if "linux" ~= os.get() and nil == _OPTIONS["gcc"] then
		print("GCC flavor must be specified!")
		os.exit(1)
	end

	if "mingw" == _OPTIONS["gcc"] then
		premake.gcc.cc = "$(MINGW)/bin/mingw32-gcc"
		premake.gcc.cxx = "$(MINGW)/bin/mingw32-g++"
		premake.gcc.ar = "$(MINGW)/bin/ar"
	end
end

flags {
	"StaticRuntime",
	"NoMinimalRebuild",
	"NoPCH",
	"NativeWChar",
--	"ExtraWarnings",
	"NoRTTI",
	"NoExceptions",
	"Symbols",
}

configuration "Debug"
	targetsuffix "Debug"

configuration "Release"
	targetsuffix "Release"

configuration { "vs*" }
	defines {
		"_HAS_EXCEPTIONS=0",
		"_HAS_ITERATOR_DEBUGGING=0",
		"_SCL_SECURE=0",
		"_CRT_SECURE_NO_WARNINGS",
		"_CRT_SECURE_NO_DEPRECATE",
	}

configuration { "x32", "vs*" }
	defines { "WIN32" }
	targetdir (BUILD_DIR .. "win32_" .. _ACTION .. "/bin")
	objdir (BUILD_DIR .. "win32_" .. _ACTION .. "/obj")
	includedirs { THIRD_PARTY_DIR .. "msinttypes" }

configuration { "x64", "vs*" }
	defines { "WIN32" }
	targetdir (BUILD_DIR .. "win64_" .. _ACTION .. "/bin")
	objdir (BUILD_DIR .. "win64_" .. _ACTION .. "/obj")
	includedirs { THIRD_PARTY_DIR .. "msinttypes" }

configuration { "x32", "mingw" }
	defines { "WIN32" }
	targetdir (BUILD_DIR .. "win32_mingw" .. "/bin")
	objdir (BUILD_DIR .. "win32_mingw" .. "/obj")

configuration { "x64", "mingw" }
	defines { "WIN32" }
	targetdir (BUILD_DIR .. "win64_mingw" .. "/bin")
	objdir (BUILD_DIR .. "win64_mingw" .. "/obj")

configuration { "x32", "linux" }
	targetdir (BUILD_DIR .. "linux32" .. "/bin")
	objdir (BUILD_DIR .. "linux32" .. "/obj")

configuration { "x64", "linux" }
	targetdir (BUILD_DIR .. "linux64" .. "/bin")
	objdir (BUILD_DIR .. "linux64" .. "/obj")

configuration {} -- reset configuration

project "map2sym"
	uuid "4ca781e0-b7d8-11e0-aff2-0800200c9a66"
	kind "ConsoleApp"

	includedirs {
		"../include",
	}

	files {
		"../include/**.h",
		"../tool/**.cpp",
		"../tool/**.h",
	}

