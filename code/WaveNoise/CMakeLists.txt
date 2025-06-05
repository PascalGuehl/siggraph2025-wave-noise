cmake_minimum_required(VERSION 3.0)

project(Wave_Demos LANGUAGES CXX)

add_definitions("-DDEF_SHADERS_PATH=${CMAKE_CURRENT_SOURCE_DIR}/")

set( easycppogl_DIR "../easycppogl/Install/EasyCppOGL/lib/cmake/easycppogl")
find_package(easycppogl)

set(CMAKE_DEBUG_POSTFIX "_d")

macro(compile)
  add_executable(${ARGV0} ${ARGV0}.cpp)
  target_link_libraries(${ARGV0} easycppogl)
endmacro()

set( demos_list 
vbo_vao
)

foreach(example ${demos_list})
  compile(${example})
endforeach()

# nice vs solution
set_target_properties(${demos_list} PROPERTIES FOLDER Demos)

