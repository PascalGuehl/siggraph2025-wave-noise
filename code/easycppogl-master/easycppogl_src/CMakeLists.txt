cmake_minimum_required(VERSION 3.0)
project(easycppogl LANGUAGES C CXX)

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

set(HEADER_FILES
gl3w.c
imconfig.h
imgui.h
imgui_impl_glfw.h
imgui_impl_opengl3.h
imgui_internal.h
imstb_rectpack.h
imstb_textedit.h
imstb_truetype.h
portable_file_dialogs.h

gl_eigen.h
ebo.h
vbo.h
vao.h
shader_program.h
transform_feedback.h
texture.h
texture1d.h
texture2d.h
texture3d.h
texturecubemap.h
texture1darray.h
texture2darray.h
texturebuffer.h
fbo.h
camera.h
gl_viewer.h
mframe.h
mesh.h
ubo.h
ssbo.h
)

set(SOURCE_FILES
gl3w.c
imgui.cpp
imgui_draw.cpp
imgui_impl_glfw.cpp
imgui_impl_opengl3.cpp
imgui_widgets.cpp

vao.cpp
gl_eigen.cpp
shader_program.cpp
transform_feedback.cpp
texture1d.cpp
texture2d.cpp
texture3d.cpp
texturecubemap.cpp
texture1darray.cpp
texture2darray.cpp
texturebuffer.cpp
fbo.cpp
camera.cpp
gl_viewer.cpp
mesh.cpp
)

if (WIN32 OR APPLE)
    set (LIBS "Eigen3::Eigen;glfw;assimp::assimp;${CMAKE_DL_LIBS}")
else()
    set (LIBS "Eigen3::Eigen;glfw;${ASSIMP_LIBRARIES};${CMAKE_DL_LIBS} -lpthread")
endif()


add_library(${PROJECT_NAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)
target_compile_definitions(${PROJECT_NAME} PUBLIC "_USE_MATH_DEFINES;NOMINMAX")
target_compile_definitions(${PROJECT_NAME} PUBLIC "-DDEF_DATA_PATH=${SYS_DATA_PATH}")

if(APPLE)
	target_link_directories(${PROJECT_NAME} PUBLIC "/usr/local/lib")
endif()



target_include_directories(${PROJECT_NAME} PUBLIC  
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>  
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>  
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}> 
)

target_compile_definitions(easycppogl PUBLIC "DEF_DATA_PATH=${SYS_DATA_PATH}")
target_compile_definitions(easycppogl PUBLIC "DDEF_SHADERS_PATH=${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(easycppogl PUBLIC "DDEF_DATA_PATH=${CMAKE_CURRENT_SOURCE_DIR}")

if(MSVC)
	target_compile_options(${PROJECT_NAME} PUBLIC /arch:AVX2)
    target_compile_options(${PROJECT_NAME} PRIVATE /Z7)  # no pdb
else()
	target_compile_options(${PROJECT_NAME} PUBLIC -mAVX2)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS})


# Create of an easycppogl cmake package

set(${PROJECT_NAME}_VERSION 1.0.0)
set(ConfigPackageLocation lib/cmake/${PROJECT_NAME})
set(ConfigPackageLocalLocation ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/)


# install library in lib
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)


# install header in include/
install(FILES ${HEADER_FILES}
        DESTINATION include/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/GL
        DESTINATION include/${PROJECT_NAME}
        FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ${CMAKE_SOURCE_DIR}/KHR
        DESTINATION include/${PROJECT_NAME}
        FILES_MATCHING PATTERN "*.h")


include(CMakePackageConfigHelpers)

#creation of easycppoglConfigVersion.cmake
write_basic_package_version_file(
  "${ConfigPackageLocalLocation}/${PROJECT_NAME}ConfigVersion.cmake"
  VERSION ${easycppogl_VERSION}
  COMPATIBILITY AnyNewerVersion
)

#creation of easycppoglTargets.cmake
export(EXPORT ${PROJECT_NAME}Targets
       FILE "${ConfigPackageLocalLocation}/${PROJECT_NAME}Targets.cmake"
       )

# creation if easycppoglConfig.cmake
configure_package_config_file( ${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
  "${ConfigPackageLocalLocation}/${PROJECT_NAME}Config.cmake"
	INSTALL_DESTINATION ${ConfigPackageLocation}
	NO_SET_AND_CHECK_MACRO
	NO_CHECK_REQUIRED_COMPONENTS_MACRO)


# install easycppoglTargets
install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${ConfigPackageLocation}
)

# install easycppoglConfig.cmake and ConfigVersion.cmake
install(FILES 
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${ConfigPackageLocation}
)
