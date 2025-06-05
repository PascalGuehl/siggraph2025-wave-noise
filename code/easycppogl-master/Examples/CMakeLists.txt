cmake_minimum_required(VERSION 3.0)

project(EasyCppOGL_Examples LANGUAGES CXX)

add_definitions("-DDEF_SHADERS_PATH=${CMAKE_CURRENT_SOURCE_DIR}/")


macro(compile)
  add_executable(${ARGV0} ${ARGV0}.cpp)
  target_link_libraries(${ARGV0} easycppogl)
endmacro()

set( examples_list 
tesselation_terrain
vbo_vao
perface
mesh_texture_fbo
texture_buffer
implicit_geom
tesselation_curve
tesselation_tri_quad
tesselation_bezier_tri2
compute
compute_textures
compute_vbos
animation
spiral
tbn
env_map
deferred
obj_viewer
view_normals
)

foreach(example ${examples_list})
  compile(${example})
endforeach()

# nice vs solution
set_target_properties(${examples_list} PROPERTIES FOLDER Examples)

