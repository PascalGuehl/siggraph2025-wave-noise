# EasyCppOGL

## Compiling

### Dependencies
* eigen3
* glfw3
* assimp

#### Linux
Check driver with `glxinfo | more` (`apt install mesa-utils` if not installed)
Install dependencies if needed `apt install libeigen3-dev libglfw3-dev libassimp-dev`
And dev tools if you have not `apt install g++ cmake cmake-curses-gui`

Compile the library
``bash
mkdir build_EasyCPPOGL
cmake -S EasyCPPOGL -B build_EasyCPPOGL -DCMAKE_BUILD_TYPE=Release
``
Or you can use ccmake to see and modify options, or even use QtCreator integrated cmake project manager.


#### Windows

Use vckpg (install follow https://vcpkg.io/en/getting-started.html)

Use triplet x64-windows (var env: VCPKG_DEFAULT_TRIPLET=x64-windows)

Use cmake-gui version:
* Generator Visual Studio 15 2017 minimum
* Optional platform: x64
* Specificy toolchain file : C:\VCPKG\scripts\buildsystems\vcpkg.cmake (changee path to yours)

Or by command line
	cmake.exe ..\easycppogl\ -DCMAKE_GENERATOR_PLATFORM=x64 
	-DCMAKE_TOOLCHAIN_FILE="C:\VCPKG\scripts\buildsystems\vcpkg.cmake"

Do not forget to add path of your installation bins `C:\VCPKG\installed\x64-windows\bin` and
`C:\VCPKG\installed\x64-windows\debug\bin` to your **PATH** environment variable


## Installing
The library can be installed and use by external code very easylly 

### installation
During the cmake configuration, take care of modifying the CMAKE_INSTALL_PREFIX,
and choose a path with writing rights.
Build the install target in debug and release mode. 

### Using installed version
In your CMakeLists.txt project, use `find_package(easycppogl)` then just link with easycppogl!

Before CMake configuration add var `CMAKE_PREFIX_PATH` with the installation path. That allow 
cmake to fill itself the `easycppogl_DIR` var.

The othery solution is to fill `easycppogl_DIR` with path where are the file cmakeConfig.cmake and CMakeTargets.cmake. Is is in `/lib/cmake` in installation path.
It could work also directly in building three of the lib (in `easycppogl_src/easycppogl`).

# Library Documentation	
## Generality
### Memory management and pointers
We use std::unique_ptr and std::shared_ptr for easy memory management. Each class has a type shortcut named UP or SP (respectively for unique and shared pointer). For example:
```C++
ShaderProgram::UP
VBO::SP
```

They can be used as a pointer, just do not use delete.  SP calls automatically deleter when all instances (copies) of the pointer have disappeard. a UP can not ne copied and objet will be freed when pointer disappeard (is freed). It is like an object that can not be copied.

Remark: It is possible to get a raw pointer with the *get* method (`sptr.get()`), but it is not recommanded if you store the pointer or work with asynchronus threads. You should not have to used it for simple programs.

You can always use any functionality of openGL directly. All Object classes have an *id()* method that give you the id of buffers, and a *bind()* method.

### Vectors and matrices
For vectors and matrices, computation and storage, we use the Eigen library. It is very efficient, its  only drawback is that it can be difficult to debug/compile, due to
complexe type representation.
If you have type problem when using for example 
```C++ 
GLVec3 v1 = ...;
GLVec3 v2 = ...;
f(v1+v2)
`` 
It is because the type of expression `v1+v2` is not a GLVec3, sometimes conversion cannot
be done by compiler. To avoid this problem use `(v1+v2).eval()` or store expression in a 
temporary variable of the good type (not use _auto_ here).

For simple usage, please use the shortcuts:
* *GLVec2, GLVec3,GLVec4* : for vectors of 2,3,4 dimensions
* *GLMat2, GLMat3, GLMat4* : for matrices 2x2 x3 and 4x4
* *GLVVecX can be use instead of std::vector<GLVecX>

Remark: Vectors are 1-column matrices, for a simple output on a line transpose it:
``C++
std::cout << v1.transpose()<< std::endl;
``

## Shader programs
### creation
To create a shader call the *create* static function with a vector of pair (shader_type,source), and a name as parameters. name and source are std::string For example:

```C++
 ShaderProgram::UP prg = ShaderProgram::create({{GL_VERTEX_SHADER,phong_vert},{GL_FRAGMENT_SHADER,phong_frag}}, "phong_shader");
 ```

sources strings can be loaded from files with the function 

The name is used to print shader's name when there are compiling errors.

### set uniform values
Use the simple template function *set_uniform_value(location,value)*

* The good gl typed function (*glUniform3f* for example) is automatically deduced from the value parameter (no time cost, except compile time !).

* Warning: location cans be replaced by the name (const std::string& str), but this call _glGetLocation_, very easy to use but not efficient (time consumming). Prefer using #version 430 with location (work on mac!)
* You must bind the shader before using the function

```C++
	prg1->bind();
	set_uniform_value(1,proj*mv);
	set_uniform_value(2,GLVec3{1,1,0});
```

### Other methods

* *bind()*
* *unbind()* static
* *id()* return a GLint

## UBO
An Uniform Buffer Objet, is a buffer that can be shared across many shader and updated in one call whatever it size which is limited in practise to 65536 bytes
Here we  used ahder *#version 430" to allow inline binding

### creation
Create the UBO with static function *create*

```C++
struct OBJ
{
	GLMat4 view;
	GLMat4 proj;
};
OBJ obj1;
UBO::UP ubo1 = UBO::create(obj1, 3);
```

### update
To update contain of shared buffer, call the *update* methods,
with a pointer of anything and optional offset and size for partial update.
* *update(ptr)*
* *update(ptr,offset,size_in_bytes)*


### other methods:
* *id()*
* *bind()*
* *unbind* static

## VBO
### creation
use static functions **create** 
 * *create(dim)* dim is the dimension of elements of BVO,
 1,2,3,4 respectively for float vec2,vec3,vec4.
 
 * *create(vector<T>)* : with *T* a float GLVec2 GLVec3 GLVec4

```C++
auto vbo_c = VBO::create(3);
auto vbo_p = VBO::create(GLVVec3{ {0,0,0},{0.5,0,0},{0.5,0.5,0},{0,0.5,0} });
```

### memory alloction
Method *allocate(nb)* allocate nb elements (the type: float, vec2, vec3 or vec4, depend on creation)

```C++
 vbo_c->allocate(4);
```

#### update
To modify content of the VBO use the update methods:

The vector update method:
 `update(std::vector<T> data, [first, nb])  `  with T : float, vec2, vec3 or vec4.

 *firt* and *nb* are optional. Warning, always take care to furnish enough data.

For examples:
```C++
vbo_p->update({0.7f, 0.6f}, 6, 2); // update de 2 float en 6,7
vbo_p->update(GLVVec3{{0, 0, 0}},0,1); // update du Vec3 en 0
```

Or more genaly you can use the pointer update methods
`update(T* data, first, nb)  `  with T : float, vec2, vec3 or vec4, 

first and nb are here relatives to the type of data (not in bytes like in openGL calls)

Other methods:
* *id()*
* *bind()*
* *unbind()* static


## EBO
Same as **VBO** but for integer buffer. Used to store indices indirection to position/normal/... in mesh definition.

### creation
Use static mathod `EBO::SP create(std::vector<GLuint> buffer)`
### update
`update (std::vector<GLuint> buffer)` copy buffer data in the EBO 
### other methods
* *allocate(nb)* allow to resize the EBO
* *id()*
* *bind()*
* *unbind()* static
* *length()* number of elements in VBO

## VAO

### creation
`VAO::UP create(const std::vector<std::tuple<GLint,VBO::SP>>& att_vbo)`:  param is a vector of pairs attribute location and vbo 

`VAO::UP create(const std::vector<std::tuple<GLint,VBO::SP,GLint>>& att_vbo)`: param is a vector of triplet attribute location, vbo, and attribute divisor 

### other methods
* *id()*
* *bind()*
* *unbind()* static
* *length()*: min number of elements of the non-instance vbos.
* *bind_none()* static : bind to an empty vao (usefull for procedural shader with non geometric attribute)

## Texture
### Common methods
* *id()*
* *bind()*
* *unbind()* static
* *bind(engine_number)* : Activate texture engine and bind texture, return sampler uniform to affect to shader with *set_uniform_value*
* *simple_params({p1,p2,..})* : p1,p2,... are last parameter of *glTexParameteri()* second one deduced automatically for basic usage. For specific usage use *glTexParameteri* calls between bind and unbind
* *create(params)* static methods of each texture class. Return a TextureXX::SP. params is a vector of enum same as for *simple_params*. If leaved empy, default is set to {GL_LINEAR,GL_CLAMP_TO_EDGE}.

### Texture1D
* *alloc(w,internal_format,ptr_on_data)*
### Texture2D
* *alloc(w,h,internal_format,ptr_on_data)*
* *init(internal)* : iniitialize internal format only, no allocation, usefull for FBO
* *resize(w,h)* : usefull for FBO
* *load(filename,force_nb_channels)* load an image into texture. Internal and external format are deduce from image. Last parameter is optionnal 1,3,4 to force channels number.
* _update(GLint x, GLint y, GLint w, GLint h, const T* data))_ : update a part of texture. Warning T must be compatible with texture format.
  
### Texture3D

* *alloc(w,h,d,internal_format,ptr_on_data)*
* _update(GLint x, GLint y, GLint z, GLint w, GLint h, GLint d, const T* data))_ : update a part of texture. Warning T must be compatible with texture format.

### Texture1DArray
* *alloc(n,w,internal_format,ptr_on_data)*
* _update(GLint ind, GLint x, GLint w, const T* data))_ :: update a part of one texture of the array. Warning T must be compatible with texture format.

### Texture2DArray
* *alloc(n,w,h,internal_format,ptr_on_data)*
* _update(GLint ind, GLint x, GLint y, GLint w, GLint h, const T* data))_ : update a part of one texture of the array. Warning T must be compatible with texture format.


### TextureCubeMap
* *alloc(w,h,d,internal_format,ptr_on_data)*
* _update(GLint ind, GLint x, GLint y, GLint w, GLint h, const T* data))_ : update a part of one of the 6 images texture of texture. Warning T must be compatible with texture format.

### *TextureBuffer
* alloc(w,h,internal_format,ptr_on_data)

## FBO
### Different FBOs

* FBO : a simple frame buffer object with only color attachments
* FBO_Depth: a frame buffer object with depth buffer
* FBO_Depth_Texture : a frame buffer object with depth buffer in a gettable texture
* FBO_Read() TODO
### creations
* *FBO::create(const std::vector<Texture2D::SP>& attach)* attach is a vector of buffer attached to GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, ...GL_COLOR_ATTACHMENTn
* *FBO_Depth::create(const std::vector<Texture2D::SP>& attach, FBO_Depth::SP from)* : add a Zbuffer that can be optionnaly shared from another *FBO_Depth from* if not nullptr
* *FBO_Depth_Texture::create(const std::vector<Texture2D::SP>& attach, Texture2D::SP dt)* : add a *Texture2D* Zbuffer that can be optionnaly shared if *dt* not nullptr


### other methods
* *id()*
* *bind()*
* *push()* save viewport & drawbuffer states
* *void pop()* restore viewport & drawbuffer states, warning do not replace bind
* *resize(w,h)*
* *width()*
* *height()*
* *texture(i)* return the texture associated with GL_COLOR_ATTACHMENTi
* *nb_textures()*
* *depth_texture()* return the depth texture (Texture2D::SP) for FBO_Depth_Texture, else nullptr


## SSBO
An Shader Storage Buffer Objet, is a buffer that can be shared across many shader and updated in one call whatever it size which is unlimited
shader *#version 430" is necessay

### creation
Create the SSBO with static function *create(size_in_byte, binding_point,pointer on data)*

### update
use the method *update(pointer,offset_begin,size_in_byte)*

### other methods:
* *id()*
* *bind()*
* *unbind* static


## BoundingBox
methods:
* 
* *add_point(const GLVec3& P)* 
* *min()* return min point of BB
* *max()* return max point of BB
* *center()* return center pf BB
* *radius()* return the radius of BB
* *matrix()* return the matrix of transform of BB



## Mesh
 A mesh is a structure that store data and generate VBOs, EBOs, VAOs, and all necessary to draw a mesh in one call

### creation
Some static creation fonctions, that return a `Mesh::SP` (shared_ptr). The mesh BB is in [-1,+1]
* CubePosOnly()
* Cube()
* Grid(GLint m=4, GLint n=4)
* Wave(GLint m);
* Sphere( GLint n)
* Cylinder(int sides, float radius_ratio);

### loading mesh file
The mesh can be created from a fille, with the static method load(filename) which return a vector of meh
because the lib assimpt can have to subdivide the mesh to ensure the importation. For simple objects, the returned 
vector containd on'rrs
A mesh it Self can be shared without any problem because it does not contain any GL data.
Drawing can be done through a **MeshRenderer** or **InstancedMeshRenderer** object which cannot be shared (used **::UP**), 
because it contains VAO, VBO, ...

To create a MeshRenderer use Mesh method :
``renderer(position_attribute_al, normal_al, texture_coord_al_,tangents_al,color_al);``
*al* mean attribute_location. All 5 parameters are integers with default values -1 when they are not used in the shader
For example to generate a renderer that use postions attibute at location 1 and normals at location 2 call
``MeshRenderer::UP myrenderer1 = mymesh->renderer(1,2,-1,-1,-1); ``

To create a InstancedMeshRenderer use Mesh method :
``instanced_renderer(inst_vbos, position_attribute_al, normal_al, texture_coord_al_,tangents_al,color_al);``
inst_vbo is a vector of tuple<int,VBO::SP,int> (attribute_location of shader, VBO shared_pointer, attribute divisor)
Other params are same than for anon-instanced version
``InstancedMeshRenderer::UP myrenderer2 = mymesh->renderer({{11,ivbo1,1},{12,ivbo2,5}},1,2,-1,-1,-1); ``
A tuple of 3 values, can be declared by ``{11,ivbo,1}`` just put `{ }` around, and separate by `, ` to have a vector

## GLViewer

To do an application, the most simple way is to inherit from GLViewer class, and to overwrite some virtual methods:
* *init_ogl()*
* *draw_ogl()*
* *interface_ogl()*
* *resize_ogl(w,h)*
* *close_ogl()*
* *mouse_release_ogl(b,x,y)*
* *mouse_press_ogl(b,x,y)*
* *mouse_mouve_ogl(x,y)*
* *mouse_wheel_ogl(x,y)*
* *key_press_ogl(key_code)*;
* *key_release_ogl(key_code)*;

### other methods
* *get_time()* get number of seconds (double) sped since beginning of the lanch of the app
* *lock()*
* *unlock()*
* *Camera* return an unique_ptr of the camera of the viewer

### short-cuts to camera methods
* *get_projection_matrix()*
* *get_view_matrix()* 
* *set_scene_radius(r)*
* *set_scene_center(p)*

## Camera
* *width()*
* *height()*
* *set_type(PERSPECTIVE | ORTHOGRAPHIC)*
* *set_field_of_view(angle)*
* *field_of_view()* return the FOV
* *set_aspect_rati(aspect) aspect is h/w
* *set_scene_radius(r)*
* *get_projection_matrix()*
* *get_view_matrix()* return the viewmatrix
* *change_pivot_point(P)* change the point to consider as the center of the scene
* *center_scene()* place the pivot point at the center of the screen.
* *show_entire_scene()*
* *reset()* 
* *scene_radius()*
* *pivot_point()* return pivot points in GLVec3d
* *pivot_point_f()* return pivot points in GLVec3f 
* *focal_dist()*
* *z_near()*
* *z_far()*
* *look_dir(eye, dir, up)*

