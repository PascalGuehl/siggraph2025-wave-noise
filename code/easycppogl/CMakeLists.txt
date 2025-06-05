cmake_minimum_required(VERSION 3.0)
project(EasyCppOGL)

option(BUILD_DEMOS "Build some example apps." ON)
if (WIN32)
	set (SYS_DATA_PATH $ENV{HOME} CACHE PATH "Path where to read image mesh ...")
else()
	set (SYS_DATA_PATH $ENV{HOMEPATH} CACHE PATH "Path where to read image mesh ...")
endif()
set_property(GLOBAL PROPERTY USE_FOLDERS ON) 
set(CMAKE_DEBUG_POSTFIX "_d")

add_subdirectory(easycppogl_src)

if (${BUILD_DEMOS})
	add_subdirectory(Demos)
endif()
