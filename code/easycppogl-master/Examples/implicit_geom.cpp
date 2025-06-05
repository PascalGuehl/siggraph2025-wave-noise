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


static const std::string imp_vert = R"(
#version 330
#extension GL_ARB_explicit_uniform_location : enable

layout(location=0) uniform mat4 vMatrix;
layout(location=2) uniform float t;
flat out int id;
void main()
{
	id = gl_VertexID%3;
	float u = float(gl_VertexID%100) / 100.0;
	float a =  u * 6.2830 + t;
	float z =-2.0+ float(gl_VertexID) / 5000.0;
	float r = 0.5 + 0.1*sin(float(gl_VertexID) / 1000.0);
	vec2 p = r*vec2(cos(a) , sin(a));
	gl_Position = vMatrix*vec4(p,z,1);
}
)";


static const std::string imp_geom = R"(
#version 330
#extension GL_ARB_explicit_uniform_location : enable

layout(location=1) uniform mat4 pMatrix;
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
flat in int id[];
out vec2 lc;
flat out int col;

void main()
{
	col = id[0];
	lc = vec2(-1,-1);
	gl_Position = pMatrix*(gl_in[0].gl_Position + vec4(-0.01, -0.01,0,0));
	EmitVertex();
	lc = vec2(1,-1);
	gl_Position = pMatrix*(gl_in[0].gl_Position + vec4(0.01, -0.01,0,0));
	EmitVertex();
	lc = vec2(-1,1);
	gl_Position = pMatrix*(gl_in[0].gl_Position + vec4(-0.01, 0.01,0,0));
	EmitVertex();
	lc = vec2(1,1);
	gl_Position = pMatrix*(gl_in[0].gl_Position + vec4(0.01,0.01,0,0));
	EmitVertex();
	EndPrimitive();
}
)";

static const std::string imp_frag = R"(
#version 330
#extension GL_ARB_explicit_uniform_location : enable

in vec2 lc;
flat in int col;
out vec3 frag_out;
void main()
{ 
	float r2 = dot(lc,lc);
	if (r2 > 1.0)
		discard;
	vec3 color = vec3(0);
	color[col] = 0.999-r2;
	frag_out = color;
}
)";


class Viewer : public GLViewer
{
	std::shared_ptr<VAO> vao_pc;
	std::shared_ptr<ShaderProgram> prg1;
	std::shared_ptr<ShaderProgram> prg2;


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
	prg1 = ShaderProgram::create({{GL_VERTEX_SHADER,imp_vert},
								  {GL_GEOMETRY_SHADER,imp_geom},
								  {GL_FRAGMENT_SHADER,imp_frag}},
								 "pos_geom_implicite");
	set_scene_center(GLVec3(0,0,0));
	set_scene_radius(1.3);
}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	prg1->bind();
	//using uniform layout location
	set_uniform_value(0, mv); // 0 is "vMatrix"
	set_uniform_value(1, proj); // 1 is "pMatrix"
	set_uniform_value(2, current_time()/5); // 2 is "t"
	VAO::none()->bind();
	glDrawArrays(GL_POINTS, 0, 20000);
}




