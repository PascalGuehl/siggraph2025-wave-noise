cmake_minimum_required(VERSION 3.0)
project(EasyCppOGL)

option(BUILD_TEST "Build test." ON)
option(BUILD_EXAMPLES "Build some example apps." ON)
option(BUILD_TUTO "Build tutorial apps." ON)
if (WIN32)
	set (SYS_DATA_PATH $ENV{HOME} CACHE PATH "Path where to read image mesh ...")
else()
	set (SYS_DATA_PATH $ENV{HOMEPATH} CACHE PATH "Path where to read image mesh ...")
endif()
set_property(GLOBAL PROPERTY USE_FOLDERS ON) 
set(CMAKE_DEBUG_POSTFIX "_d")




add_subdirectory(easycppogl_src)

if (${BUILD_EXAMPLES})
	add_subdirectory(Examples)
endif()

if (${BUILD_TEST})
	add_subdirectory(Test)
endif()

if (${BUILD_TUTO})
	add_subdirectory(Tuto)
endif()
