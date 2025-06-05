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
#include "fbo.h"
#include "gl_viewer.h"
#include <GLFW/glfw3.h>

#define macro_str(s) #s
#define macro_xstr(s) macro_str(s)
#define DATA_PATH std::string(macro_xstr(DEF_DATA_PATH))

using namespace EZCOGL;

static const std::string pass1_vert = R"(
#version 430
layout(location=1) in vec3 position_in;
layout(location=2) in vec3 normal_in;
out vec3 P;
out vec3 N;
out vec3 C;
layout(location=1) uniform mat4 projectionMatrix;
layout(location=2) uniform mat4 viewMatrix;
layout(location=3) uniform mat4 modelMatrix;
layout(location=4) uniform mat3 modelNormalMatrix;
layout(location=5) uniform vec3 color;
void main()
{
	vec4 P4 =  modelMatrix * vec4(position_in, 1.0);
	P = P4.xyz;
	N = modelNormalMatrix * normal_in;
	C = color;
	gl_Position = projectionMatrix * viewMatrix * P4;
}
)";

static const std::string pass1_frag = R"(
#version 430
precision highp float;
in vec3 P;
in vec3 N;
in vec3 C;

layout(location = 0) out vec3 P_out;
layout(location = 1) out vec3 N_out;
layout(location = 2) out vec3 C_out;

void main()
{
	P_out = P;
	N_out = normalize(N);
	C_out = C;
}
)";

static const std::string pass2_vert = R"(
#version 430
void main()
{
	vec2 P = vec2(gl_VertexID%2, gl_VertexID/2);
	gl_Position = vec4(2.0*P-1.0,0,1);
}
)";

static const std::string pass2_frag = R"(
#version 430
precision highp float;
layout(binding=0) uniform sampler2D tP;
layout(binding=1) uniform sampler2D tN;
layout(binding=2) uniform sampler2D tC;
layout(location=1) uniform vec3 lightP ;
layout(location=2) uniform vec3 lightC ;
out vec3 frag;

void main()
{
	ivec2 coord = ivec2(gl_FragCoord.xy);
	
	vec3 P = texelFetch(tP,coord,0).rgb;
	vec3 N = texelFetch(tN,coord,0).rgb;
	vec3 C = texelFetch(tC,coord,0).rgb;
	vec3 L = (lightP-P);
	float d2 = dot(L,L);
	L /= sqrt(d2);
	float lamb = 0.05+0.95*max(0.0,dot(L,N));
	frag = lamb * lightC * C ;
}
)";

class Viewer: public GLViewer
{
	ShaderProgram::UP prg1_;
	ShaderProgram::UP prg2_;
	MeshRenderer::UP renderer1_;
	MeshRenderer::UP renderer2_;
	FBO_Depth::SP fbo_;
public:
	Viewer();
	void init_ogl() override;
	void draw_ogl() override;
	void resize_ogl(int32_t w, int32_t h) override;
};

int main(int, char**)
{
	Viewer v;
	return v.launch3d();
}

Viewer::Viewer() :
	prg1_(nullptr),
	prg2_(nullptr),
	renderer1_(nullptr),
	renderer2_(nullptr),
	fbo_(nullptr)
{}

void Viewer::init_ogl()
{
	prg1_ = ShaderProgram::create({ {GL_VERTEX_SHADER,pass1_vert},{GL_FRAGMENT_SHADER,pass1_frag} }, "pass1");
	prg2_ = ShaderProgram::create({ {GL_VERTEX_SHADER,pass2_vert},{GL_FRAGMENT_SHADER,pass2_frag} }, "pass2");

	auto me = Mesh::Wave(80);
	renderer1_ = me->renderer(1, 2, -1, -1, -1);
	me = Mesh::Sphere(80);
	renderer2_ = me->renderer(1, 2, -1, -1, -1);

	Texture2D::SP tP = Texture2D::create();
	tP->init(GL_RGB32F);
	Texture2D::SP tN = Texture2D::create();
	tN->init(GL_RGB32F);
	Texture2D::SP tC = Texture2D::create();
	tC->init(GL_RGB8);

	fbo_ = FBO_Depth::create({ tP,tN,tC });

	set_scene_center(GLVec3(0, 0, 0));
	set_scene_radius(5);
}

void Viewer::resize_ogl(int32_t w, int32_t h)
{
	fbo_->resize(w, h);
}

void Viewer::draw_ogl()
{
	FBO::push();
	fbo_->bind();
	glClearColor(0, 0, 0, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& view = this->get_view_matrix();

	prg1_->bind();
	set_uniform_value(1, proj);
	set_uniform_value(2, view);
	GLMat4 model = Transfo::translate(-2, -2, -0.5)*Transfo::rotateZ(45.0f);
	set_uniform_value(3, model);
	set_uniform_value(4, Transfo::inverse_transpose(model));
	set_uniform_value(5, GLVec3(0, 1, 1));
	renderer1_->draw(GL_TRIANGLES);

	model = Transfo::translate(2, -2, -0.5) * Transfo::rotateZ(55.0f);
	set_uniform_value(3, model);
	set_uniform_value(4, Transfo::inverse_transpose(model));
	set_uniform_value(5, GLVec3(1,1,0));
	renderer1_->draw(GL_TRIANGLES);

	model = Transfo::translate(2, 2, -0.5) * Transfo::rotateZ(50.0f);
	set_uniform_value(3, model);
	set_uniform_value(4, Transfo::inverse_transpose(model));
	set_uniform_value(5, GLVec3(0, 1, 0));
	renderer1_->draw(GL_TRIANGLES);

	model = Transfo::translate(-2, 2, -0.5) * Transfo::rotateZ(40.0f);
	set_uniform_value(3, model);
	set_uniform_value(4, Transfo::inverse_transpose(model));
	set_uniform_value(5, GLVec3(1, 0, 0));
	renderer1_->draw(GL_TRIANGLES);

	model = Transfo::translate(-2, -2, 1) * Transfo::scale(0.5f);
	set_uniform_value(3, model);
	set_uniform_value(4, Transfo::inverse_transpose(model));
	set_uniform_value(5, GLVec3(0, 1, 1));
	renderer2_->draw(GL_TRIANGLES);

	model = Transfo::translate(2, -2, 1) * Transfo::scale(0.5f);
	set_uniform_value(3, model);
	set_uniform_value(4, Transfo::inverse_transpose(model));
	set_uniform_value(5, GLVec3(1, 1, 0));
	renderer2_->draw(GL_TRIANGLES);

	model = Transfo::translate(2, 2, 1) * Transfo::scale(0.5f);
	set_uniform_value(3, model);
	set_uniform_value(4, Transfo::inverse_transpose(model));
	set_uniform_value(5, GLVec3(0, 1, 0));
	renderer2_->draw(GL_TRIANGLES);

	model = Transfo::translate(-2, 2, 1) * Transfo::scale(0.5f);
	set_uniform_value(3, model);
	set_uniform_value(4, Transfo::inverse_transpose(model));
	set_uniform_value(5, GLVec3(1, 0, 0));
	renderer2_->draw(GL_TRIANGLES);

	FBO::unbind();
	FBO::pop();
	glDisable(GL_DEPTH_TEST);
	prg2_->bind();
	fbo_->texture(0)->bind(0);
	fbo_->texture(1)->bind(1);
	fbo_->texture(2)->bind(2);
	set_uniform_value(1, GLVec3(0, 0, 15));
	set_uniform_value(2, GLVec3(0.9,0.9,0.9));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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
