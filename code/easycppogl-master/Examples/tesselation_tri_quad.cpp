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
#include "texture2d.h"

#define macro_str(s) #s
#define macro_xstr(s) macro_str(s)
#define DATA_PATH std::string(macro_xstr(DEF_DATA_PATH))


using namespace EZCOGL;


static const std::string simple_vert = R"(
#version 430
layout(location=0) in vec3 P;
void main()
{
	gl_Position = vec4(P,1);
}
)";


static const std::string tri_ctrl = R"(
#version 430
layout (vertices = 3) out;
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
out vec3 color;
uniform mat4 pvMatrix;

void main()
{
	vec3 Q = gl_TessCoord.x * gl_in[0].gl_Position.xyz;
	Q += gl_TessCoord.y * gl_in[1].gl_Position.xyz;
	Q += gl_TessCoord.z * gl_in[2].gl_Position.xyz;

	gl_Position = pvMatrix * vec4(Q,1);
	color = vec3(gl_TessCoord.xy,0.0);
}
)";

static const std::string simple_frag = R"(
#version 430
in vec3 color;
out vec3 frag_out;
void main()
{ 
	frag_out = color;
}
)";



static const std::string quad_ctrl = R"(
#version 430
layout (vertices = 4) out;
uniform int outer;
uniform int inner;
void main()
{
	gl_TessLevelOuter[0] = outer;
	gl_TessLevelOuter[1] = outer;
	gl_TessLevelOuter[2] = outer;
	gl_TessLevelOuter[3] = outer;

	gl_TessLevelInner[0] = inner;
	gl_TessLevelInner[1] = inner;

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)";

static const std::string quad_eval = R"(
#version 430

layout (quads, equal_spacing, ccw) in;
out vec3 color;
uniform mat4 pvMatrix;
uniform sampler2D TU;

void main()
{
	vec3 Qb = mix(gl_in[0].gl_Position.xyz,gl_in[1].gl_Position.xyz, gl_TessCoord.x);
	vec3 Qt = mix(gl_in[3].gl_Position.xyz,gl_in[2].gl_Position.xyz, gl_TessCoord.x);
	vec3 Q = mix(Qb,Qt, gl_TessCoord.y);
	gl_Position = pvMatrix * vec4(Q,1);
	color = texture(TU,gl_TessCoord.xy).rgb;
}
)";



class Viewer : public GLViewer
{
	std::shared_ptr<VAO> vao_pn;
	std::shared_ptr<ShaderProgram> prg_tri;
	std::shared_ptr<ShaderProgram> prg_quad;
	Texture2D::SP tex;

	int tess_outer_;
	int tess_inner_;

public:
	Viewer();
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
	tess_outer_(5), tess_inner_(4)
{}


void Viewer::init_ogl()
{
	auto vbo_p = VBO::create(GLVVec3{{-2,-2,0},{0,-2,0},{-0.25,0,0},{-1.5,0,0},{0,2,0},{1,0,0 },{2,2,0}});

	vao_pn = VAO::create({{0,vbo_p}});

	prg_tri = ShaderProgram::create({{GL_VERTEX_SHADER,simple_vert},
										 {GL_TESS_CONTROL_SHADER,tri_ctrl},
										 {GL_TESS_EVALUATION_SHADER,tri_eval},
										 {GL_FRAGMENT_SHADER,simple_frag}},
										 "tri_tess");

	prg_quad = ShaderProgram::create({{GL_VERTEX_SHADER,simple_vert},
										 {GL_TESS_CONTROL_SHADER,quad_ctrl},
										 {GL_TESS_EVALUATION_SHADER,quad_eval},
										 {GL_FRAGMENT_SHADER,simple_frag}},
										 "quad_tess");
	tex = Texture2D::create();
	tex->load(DATA_PATH+"/chaton.png");

	set_scene_center(GLVec3(0,0,0));
	set_scene_radius(3);
}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();
	GLMat4 pvm = proj*mv;

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	vao_pn->bind();

	prg_quad->bind();
	set_uniform_value("pvMatrix", pvm);
	set_uniform_value("outer", tess_outer_);
	set_uniform_value("inner", tess_inner_);
	set_uniform_value("TU",tex->bind(0));
	glPatchParameteri( GL_PATCH_VERTICES, 4 );
	glDrawArrays( GL_PATCHES, 0, 4 );

	prg_tri->bind();
	set_uniform_value("pvMatrix", pvm);
	set_uniform_value("outer", tess_outer_);
	set_uniform_value("inner", tess_inner_);
	glPatchParameteri( GL_PATCH_VERTICES, 3 );
	glDrawArrays( GL_PATCHES, 4, 3 );
}

void Viewer::interface_ogl()
{
	ImGui::Begin("EasyCppOGL Tessellation",nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::SetWindowSize({0,0});
	ImGui::SliderInt("Outer",&tess_outer_,1,50);
	ImGui::SliderInt("Inner",&tess_inner_,1,50);

	ImGui::End();
}



