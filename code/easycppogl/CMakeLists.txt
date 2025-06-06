cmake_minimum_required(VERSION 3.0)
project(EasyCppOGL)

# Installation path
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/Install/EasyCppOGL" CACHE PATH "Installation directory" FORCE)

if (WIN32)
	set (SYS_DATA_PATH $ENV{HOME} CACHE PATH "Path where to read image mesh ...")
else()
	set (SYS_DATA_PATH $ENV{HOMEPATH} CACHE PATH "Path where to read image mesh ...")
endif()
set_property(GLOBAL PROPERTY USE_FOLDERS ON) 
set(CMAKE_DEBUG_POSTFIX "_d")

add_subdirectory(easycppogl_src)
