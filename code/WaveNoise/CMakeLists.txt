cmake_minimum_required(VERSION 3.0)

project(Demos LANGUAGES CXX)

add_definitions("-DDEF_SHADERS_PATH=${CMAKE_CURRENT_SOURCE_DIR}/")

set( easycppogl_DIR "../easycppogl/Install/EasyCppOGL/lib/cmake/easycppogl")
find_package(easycppogl REQUIRED)

set(CMAKE_DEBUG_POSTFIX "_d")

#set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/Install/Demos" CACHE PATH "Install path" FORCE)

macro(compile)
  add_executable(${ARGV0} ${ARGV0}.cpp)
  target_link_libraries(${ARGV0} easycppogl)
  
  #install(TARGETS ${ARGV0} DESTINATION bin)
endmacro()

set( demos_list 
wave_noise_01
)

foreach(example ${demos_list})
  compile(${example})
endforeach()

# nice vs solution
set_target_properties(${demos_list} PROPERTIES FOLDER Demos)

