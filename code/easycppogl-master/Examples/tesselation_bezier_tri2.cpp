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
#include "vbo.h"
#include "vao.h"
#include "gl_viewer.h"


using namespace EZCOGL;


static const std::string simple_vert = R"(
#version 430
layout(location=0) in vec3 P;
uniform mat4 vMatrix;
void main()
{
	gl_Position = vMatrix*vec4(P,1);
}
)";


static const std::string tri_ctrl = R"(
#version 430
layout (vertices = 6) out;
uniform int outer;
uniform int inner;
void main()
{
	gl_TessLevelOuter[0] = outer;
	gl_TessLevelOuter[1] = outer;
	gl_TessLevelOuter[2] = outer;

	gl_TessLevelInner[0] = inner;

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)";

static const std::string tri_eval = R"(
#version 430

layout (triangles, equal_spacing, ccw) in;

uniform mat4 pMatrix;
uniform float t;
uniform float h;

out vec3 P;

void main()
{
	vec3 dz = vec3(0,0,h*(sin(t+float(gl_PrimitiveID))));
	vec3 Pa = gl_in[4].gl_Position.xyz+dz;
	vec3 Pb = gl_in[2].gl_Position.xyz+dz;
	vec3 Pc = gl_in[1].gl_Position.xyz+dz;

	vec3 A = gl_TessCoord.x * gl_in[0].gl_Position.xyz;
	A += gl_TessCoord.y * Pc;
	A += gl_TessCoord.z * Pb;
	P = gl_TessCoord.x * A;
	vec3 B = gl_TessCoord.x * Pc;
	B += gl_TessCoord.y * gl_in[3].gl_Position.xyz;
	B += gl_TessCoord.z * Pa;
	P += gl_TessCoord.y* B;
	vec3 C = gl_TessCoord.x * Pb;
	C += gl_TessCoord.y * Pa;
	C += gl_TessCoord.z * gl_in[5].gl_Position.xyz;
	P += gl_TessCoord.z * C;

	gl_Position = pMatrix * vec4(P,1);
}
)";

static const std::string simple_frag = R"(
#version 430
in vec3 P;
out vec3 frag_out;

const vec3 light_pos = vec3(100,300,1000);
const vec3 color = vec3(0.8,0.1,0.4);

void main()
{
	vec3 L = normalize(light_pos-P);
	vec3 No = normalize(cross(dFdx(P),dFdy(P)));
	float lamb = 0.2+0.75*max(0.0,dot(No,L));
	vec3 E = normalize(-P);
	vec3 R = reflect(-L, No);
	float spec = pow( max(dot(R,E), 0.0), 150.0);
	frag_out = mix(color*lamb,vec3(1.0),spec);
}
)";


class Viewer : public GLViewer
{
	std::shared_ptr<VBO> vbo_p;
	std::shared_ptr<VAO> vao_bt;
	std::shared_ptr<ShaderProgram> prg_tri;
	int tess_level_;

	int nb_pc_;
	int N_;
	float h_;

public:
	Viewer();

	void init_patches();
	void init_ogl() override;
	void draw_ogl() override;
	void interface_ogl() override;
};

int main(int, char**)
{
	Viewer v;
	return v.launch3d();
}

Viewer::Viewer():
	 tess_level_(10), N_(10)
{}

void Viewer::init_patches()
{
	GLVVec3 Q{{0,2,0},
	  {-1.5f,0,0},{1.5f,0,0},
  {-2,-2,0},{0,-3,0},{2,-2,0.1f}};

	std::cout << N_ << std::endl;

	float inc = 4.0f/N_;
	h_ = inc;
	float sc = 0.8f/N_;
	GLVVec3 PC;
	PC.reserve(6*N_*N_*N_);
	for(float h=-2; h<2; h+=inc)
		for(float j=-2; j<2; j+=inc)
			for(float i=-2; i<2;i+=inc)
				for (int k=0;k<6; ++k)
				{
					GLVec3 P = GLVec3{i,j,h} + Q[k]*sc;
					PC.push_back(P);
				}
	nb_pc_ = SIZE32(PC);

	vbo_p->allocate(nb_pc_);
	vbo_p->update(PC);
}

void Viewer::init_ogl()
{
//	GLVVec3 Q{{0,2,0},
//	  {-1.5f,0,0},{1.5f,0,0},
//  {-2,-2,0},{0,-3,0},{2,-2,0.1f}};

//	GLVVec3 PC;
//	PC.reserve(6*170*40);
//	for(float h=-2; h<2; h+=0.1f)
//		for(float j=-2; j<2; j+=0.1f)
//			for(float i=-2; i<2;i += 0.1f)
//				for (int k=0;k<6; ++k)
//				{
//					GLVec3 P = GLVec3{i,j,h} + Q[k]*0.02f;
//					PC.push_back(P);
//				}
//	nb_pc_ = PC.size();

	vbo_p = VBO::create(3);
	vao_bt = VAO::create({{0,vbo_p}});
	init_patches();

	prg_tri = ShaderProgram::create({{GL_VERTEX_SHADER,simple_vert},
										 {GL_TESS_CONTROL_SHADER,tri_ctrl},
										 {GL_TESS_EVALUATION_SHADER,tri_eval},
										 {GL_FRAGMENT_SHADER,simple_frag}},
										 "tri_tess");

	set_scene_center(GLVec3(0,0,0));
	set_scene_radius(3);
}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	prg_tri->bind();
	set_uniform_value("vMatrix", mv);
	set_uniform_value("pMatrix", proj);
	set_uniform_value("outer", tess_level_);
	set_uniform_value("inner", tess_level_);
	set_uniform_value("t", current_time());
	set_uniform_value("h", h_);
	vao_bt->bind();

	glPatchParameteri( GL_PATCH_VERTICES, 6 );
	glDrawArrays( GL_PATCHES, 0, nb_pc_ );
}

void Viewer::interface_ogl()
{
	ImGui::Begin("EasyCppOGL Tessellation",nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::SetWindowSize({0,0});
	ImGui::Text("FPS :(%2.2lf)", fps_);
	ImGui::SliderInt("Inner",&tess_level_,1,40);
	int Nc = N_;
	ImGui::SliderInt("Patches",&N_,5,50);
	if (Nc != N_)
		init_patches();
	ImGui::End();
}

