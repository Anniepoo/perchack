Since libpxcutils uses STL, it is necessary generate compiler specific 
versions. This is done using a common OutputDirectory ($OutDir) definition
(lib/$(PlatformName)/$(PlatformToolset)/) within the various project files.

libpxcutils_vs2012.vcxproj -> lib/$(PlatformName)/v110
libpxcutils_vs2010.vcxproj -> lib/$(PlatformName)/v100
libpxcutils_vs2008.vcproj  -> lib/$(PlatformName)/

Note: Since 2008 doesn't define $(PlatformToolset), the 2008 files are 
located in a lib/$(PlatformName)/ instead of a v90 directory.