/*******************************************************************************
* EasyCppOGL:   Copyright (C) 2019,                                            *
* Sylvain Thery, IGG Group, ICube, University of Strasbourg, France            *
*                                                                              *
* This library is free software; you can redistribute it and/or modify it      *
* under the terms of the GNU Lesser General Public License as published by the *
* Free Software Foundation; either version 2.1 of the License, or (at your     *
* option) any later version.                                                   *
*                                                                              *
* This library is distributed in the hope that it will be useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this library; if not, write to the Free Software Foundation,      *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
*                                                                              *
* Contact information: thery@unistra.fr                                        *
*******************************************************************************/


#include <iostream>
#include "shader_program.h"
#include "mesh.h"
#include "texturecubemap.h"
#include "gl_viewer.h"
#include <GLFW/glfw3.h>

#define macro_str(s) #s
#define macro_xstr(s) macro_str(s)
#define DATA_PATH std::string(macro_xstr(DEF_DATA_PATH))

using namespace EZCOGL;

static const std::string sky_vert = R"(
#version 430
layout(location=1) in vec3 position_in;
out vec3 tc;
layout(location=1) uniform mat4 projectionviewMatrix;
void main()
{
	tc = position_in;
	vec4 P4 = projectionviewMatrix * vec4(position_in, 1.0);
	gl_Position = P4.xyww;
}
)";

static const std::string sky_frag = R"(
#version 430
precision highp float;
in vec3 tc;
out vec4 frag;
layout(location=2, binding=0) uniform samplerCube TU;
void main()
{
        frag = vec4(texture(TU, tc).rgb,1);
}

)";

static const std::string p_vert = R"(
#version 430
layout(location=1) in vec3 pos;
layout(location=1) uniform mat4 pvMatrix;

void main()
{
	gl_Position = pvMatrix*vec4(pos,1);
}
)";

static const std::string p_frag = R"(
#version 430
precision highp float;
layout(location=2) uniform vec3 color;
out vec3 frag_out;
void main()
{
	frag_out = color;
}
)";


static const std::string envir_vert = R"(
#version 430
layout(location=1) in vec3 position_in;
layout(location=2) in vec3 normal_in;
out vec3 Penv;
out vec3 Nenv;
layout(location=1) uniform mat4 projectionMatrix;
layout(location=2) uniform mat4 viewMatrix;
layout(location=3) uniform mat4 modelMatrix;
layout(location=4) uniform mat3 normalModelMatrix;
void main()
{
	Nenv = normalModelMatrix * normal_in;
	vec4 P4 =  modelMatrix * vec4(position_in, 1.0);
	Penv = P4.xyz;
	gl_Position = projectionMatrix * viewMatrix * P4;
}
)";

static const std::string envir_frag = R"(
#version 430
precision highp float;
in vec3 Penv;
in vec3 Nenv;
layout(location=8) uniform vec4 Cam;

out vec3 frag;

const vec3 Ambient = vec3(0.1);

layout(binding=0) uniform samplerCube TU;
void main()
{
	vec3 fromCam = normalize(Penv-Cam.xyz);
	vec3 N = normalize(Nenv);
	vec3 tc = reflect(fromCam,N);	
	float k = max(0.0,-dot(N,fromCam));
	frag = mix(Ambient,texture(TU, tc).rgb,k);
}
)";

class Viewer: public GLViewer
{
	ShaderProgram::UP prg_sky_;
	ShaderProgram::UP prg_col_;
	ShaderProgram::UP prg_env_;
	MeshRenderer::UP renderer_sky_;
	MeshRenderer::UP renderer_tore_env_;
	MeshRenderer::UP renderer_sphere_env_;
	TextureCubeMap::SP texcm_;
public:
	Viewer();
	void init_ogl() override;
	void draw_ogl() override;
	void key_release_ogl(int32_t keycode) override;

};

int main(int, char**)
{
	Viewer v;
	return v.launch3d();
}

Viewer::Viewer()
{}



void Viewer::init_ogl()
{
	prg_sky_ = ShaderProgram::create({ {GL_VERTEX_SHADER,sky_vert},{GL_FRAGMENT_SHADER,sky_frag} }, "sky");
	prg_col_ = ShaderProgram::create({ {GL_VERTEX_SHADER,p_vert},{GL_FRAGMENT_SHADER,p_frag} }, "col");
	prg_env_ = ShaderProgram::create({ {GL_VERTEX_SHADER,envir_vert},{GL_FRAGMENT_SHADER,envir_frag} }, "envir");


	auto me = Mesh::CubePosOnly();
	renderer_sky_ = me->renderer(1,-1,-1,-1,-1);

	texcm_ = TextureCubeMap::create();
	texcm_->load({ DATA_PATH+"/skybox1/px.jpg",DATA_PATH + "/skybox1/nx.jpg",DATA_PATH + "/skybox1/py.jpg",DATA_PATH + "/skybox1/ny.jpg",
	DATA_PATH + "/skybox1/nz.jpg",DATA_PATH + "/skybox1/pz.jpg" });

    me = Mesh::Tore(32,16,0.5f);
	renderer_tore_env_ = me->renderer(1, 2, -1, -1, -1);

	me = Mesh::Sphere(32);
	renderer_sphere_env_ = me->renderer(1, 2, -1, -1, -1);
}

void Viewer::draw_ogl()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	//set_scene_center(GLVec3(0,0,0));
	//set_scene_radius(1.0f);
	set_scene_center(GLVec3(0, 0, 0));
	set_scene_radius(6);

	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& view = this->get_view_matrix();

	GLMat4 vi = view;
        vi.block<3,1>(0, 3).setZero(); // remove translation
        vi.block<3,1>(0, 0).normalize(); //
        vi.block<3,1>(0, 1).normalize(); // remove scale
        vi.block<3,1>(0, 2).normalize(); //

	GLMat4 pv_sky = proj * vi;

	prg_sky_->bind();
        set_uniform_value(1, pv_sky*Transfo::scale(100.0f));
	texcm_->bind(0);
	renderer_sky_->draw(GL_TRIANGLES);


	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, 1.1);

	prg_env_->bind();
	set_uniform_value(1, proj);
	set_uniform_value(2, view);
	GLVec4 cam = view.inverse() * GLVec4(0, 0, 0, 1);
	//std::cout << cam.transpose() << std::endl;
	set_uniform_value(8, cam);
	texcm_->bind(0);

	GLMat4 model = Transfo::translate(-1.5f,0,0) * Transfo::rotateY(current_time() * 10.0);
	set_uniform_value(3, model);
	set_uniform_value(4, Transfo::inverse_transpose(model));
	renderer_tore_env_->draw(GL_TRIANGLES);

	model = Transfo::translate(1.5f, 0, 0) * Transfo::rotateY(current_time() * 10.0);
	set_uniform_value(3, model);
	set_uniform_value(4, Transfo::inverse_transpose(model));
	renderer_sphere_env_->draw(GL_TRIANGLES);

	//glDisable(GL_POLYGON_OFFSET_FILL);
	//prg_col_->bind();
	//set_uniform_value(1, proj * view);
	//set_uniform_value(2, GLVec3(0.9,0.9,0.3));
	//renderer_tore_->draw(GL_LINES);

}

void Viewer::key_release_ogl(int32_t keycode) {
  if (keycode == GLFW_KEY_A) {
    std::cout << "before" << std::endl << get_view_matrix() << std::endl;

    camera().look_dir(GLVec3(0, 0, -10), GLVec3(1, 1, 0), GLVec3(0, 0, 1));

    std::cout << "after" << std::endl << get_view_matrix() << std::endl;
  }
}

		
		
		
	
//void Viewer::interface_ogl()
//{
//
////	std::cout << ImGui::GetDpiScale()<<std::endl;
//
////	ImGui::GetIO().FontGlobalScale = 1.5f;
//	ImGui::Begin("Load",nullptr, ImGuiWindowFlags_NoSavedSettings);
//	ImGui::Text("FPS: %2.2lf", fps_);
//	ImGui::Text("MEM:  %2.2lf %%", 100.0 * mem_);
// 
// 
//	ImGui::SetWindowSize({0,0});
//}
//             
