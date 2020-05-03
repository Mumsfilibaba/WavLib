project "WavLib"
	kind "StaticLib"
	language "C"
	systemversion "latest"
	staticruntime "On"
	location "ProjectFiles"
	
	configdir = "%{cfg.buildcfg}-%{cfg.architecture}-%{cfg.platform}"
	
	files 
	{
		"WavLib.h",
		"WavLib.c",
	}
	
	targetdir("Build/bin/" .. configdir .. "/%{prj.name}")
	objdir("Build/bin-int/" .. configdir .. "/%{prj.name}")