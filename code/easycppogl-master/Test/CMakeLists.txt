cmake_minimum_required(VERSION 3.0)

project(EasyCppOGL_TEST LANGUAGES CXX)

set (TEST_DATA_PATH $ENV{HOME} CACHE PATH "Path where to read image mesh ...")

macro(compile)
  add_executable(${ARGV0} ${ARGV0}.cpp)
  target_link_libraries(${ARGV0} easycppogl)
endmacro()

add_definitions("-DDEF_DATA_PATH=${CMAKE_CURRENT_SOURCE_DIR}")

set( tests_list
test_shader
test_vao_vbo
test_fbo
test_ubo
test_ssbo
test_mesh
test_compute
)

foreach( exec_test ${tests_list})
	compile(${exec_test})
endforeach()

# nice vs solution
set_target_properties(${tests_list} PROPERTIES FOLDER Tests)

