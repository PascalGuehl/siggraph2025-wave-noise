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
#include "fbo.h"
#include "texture2d.h"
#include "gl_viewer.h"
#include "mesh.h"


using namespace EZCOGL;


static const std::string herm_vert = R"(
#version 430
uniform mat4 vMatrix;
uniform mat3 nMatrix;
layout(location=0) in vec3 P;
layout(location=1) in vec3 T;
out vec3 tg;
void main()
{
	gl_Position = vMatrix*vec4(P,1);
	tg = nMatrix*T;
}
)";


static const std::string herm_ctrl = R"(
#version 430
layout (vertices = 2) out;
in vec3 tg[];
out vec3 tgs[];
void main()
{
	gl_TessLevelOuter[0] = 1;
	gl_TessLevelOuter[1] = 128;
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	tgs[gl_InvocationID] = tg[gl_InvocationID];
}
)";

static const std::string herm_eval = R"(
#version 430

layout (isolines, equal_spacing, ccw) in;

uniform mat4 pMatrix;
in vec3 tgs[];
out vec3 color;

vec3 hermite(float u, vec3 p0, vec3 p1, vec3 t0, vec3 t1)
{
	float F1 = 2.*u*u*u - 3.*u*u + 1.;
	float F2 = -2.*u*u*u + 3*u*u;
	float F3 = u*u*u - 2.*u*u + u;
	float F4 = u*u*u - u*u;

	vec3 p = F1*p0 + F2*p1 + F3*t0 + F4*t1;
	return p;
}

void main()
{
	vec3 Q = hermite( gl_TessCoord.x, gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, tgs[0], tgs[1] );
	gl_Position = pMatrix * vec4(Q,1);
	color = mix(vec3(0,1,0),vec3(1,0,0),gl_TessCoord.x);
}
)";

static const std::string herm_frag = R"(
#version 430
in vec3 color;
out vec3 frag_out;
void main()
{ 
	frag_out = color;
}
)";


class Viewer : public GLViewer
{
	std::shared_ptr<VAO> vao_pn;
	std::shared_ptr<ShaderProgram> prg_hermite;

public:
	Viewer();
	void init_ogl() override;
	void draw_ogl() override;
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
	auto vbo_p = VBO::create(GLVVec3{{-2.f,0.f, 0.f },{ 0.f,0.f, 0.f }, { 0.f,0.f, 0.f },{ 2.f,0.f, 0.f }});
	auto vbo_n = VBO::create(GLVVec3{{ 0.f, 3.f, 0.f },{ 0.f, -3.f, 0.f }, { 0.f, -5.f, 0.f },{ 0.f, 5.f, 0.f }});

	vao_pn = VAO::create({{0,vbo_p},{1,vbo_n}});

	prg_hermite = ShaderProgram::create({{GL_VERTEX_SHADER,herm_vert},
										 {GL_TESS_CONTROL_SHADER,herm_ctrl},
										 {GL_TESS_EVALUATION_SHADER,herm_eval},
										 {GL_FRAGMENT_SHADER,herm_frag}},
										 "hermite_curve");
	set_scene_center(GLVec3(0,0,0));
	set_scene_radius(3);
}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	prg_hermite->bind();
	set_uniform_value("vMatrix", mv);
	set_uniform_value("nMatrix", Transfo::sub33(mv));
	set_uniform_value("pMatrix", proj);
	vao_pn->bind();
	glPatchParameteri( GL_PATCH_VERTICES, 2 );
	glDrawArrays( GL_PATCHES, 0, 4 );


}




