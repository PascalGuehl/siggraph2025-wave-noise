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
#include "gl_viewer.h"


using namespace EZCOGL;

static const std::string prg1_vert = R"(
#version 430

layout(location=1) in vec3 position_in;
layout(location=2) in vec3 color_in;

layout(location=1) uniform mat4 pmvMatrix;

out vec3 color;

void main()
{
	gl_Position = pmvMatrix *  vec4(position_in,1);
	color = color_in;
}
)";



static const std::string prg1_frag = R"(
#version 430
in vec3 color;
out vec3 frag_out;
void main()
{
	frag_out = color;
}
)";

static const std::string prg2_vert = R"(
#version 430

layout(location=1) in vec3 position_in;
layout(location=2) in vec3 color_in;
layout(location=3) in vec4 pos_inst;


layout(location=1) uniform mat4 pmvMatrix;

out vec3 color;
void main()
{
	vec3 pos = position_in * pos_inst.w + pos_inst.xyz;
	gl_Position = pmvMatrix *  vec4(pos,1);
	color = color_in;
}
)";


class Viewer : public GLViewer
{
	VAO::UP vao1_;
	VAO::UP vao2_;
	VAO::UP vao3_;
	ShaderProgram::UP prg1_;
	ShaderProgram::UP prg2_;



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

inline Viewer::Viewer()
{}


void Viewer::init_ogl()
{
	prg1_ = ShaderProgram::create({{GL_VERTEX_SHADER, prg1_vert}, {GL_FRAGMENT_SHADER, prg1_frag}}, "prg1");
	prg2_ = ShaderProgram::create({{GL_VERTEX_SHADER, prg2_vert}, {GL_FRAGMENT_SHADER, prg1_frag}}, "prg2");

	VBO::SP vbo_p = VBO::create(GLVVec3{{-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}});
	VBO::SP vbo_c = VBO::create(GLVVec3{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}});
	VBO::SP vbo_i = VBO::create(GLVVec4{{1, 1, 0, 0.6f}, {0, 2, -0.1f, 0.2f}, {2, 0, 0.1f, 0.25f},
		{2, 2, 0.2f, 0.3f}, {-2, -2, -0.2f, 0.4f}});

	vao1_ = VAO::create({{1, vbo_p}, {2, vbo_c}});

	vao2_ = VAO::create({{1, vbo_p, 0}, {2, vbo_c, 0}, {3, vbo_i, 1}});


	VBO::SP vbo_pc =
		VBO::create(GLVVec3{{-1, -1, 0}, {1, 0, 0}, {1, -1, 0}, {0, 1, 0}, {1, 1, 0}, {0, 0, 1}, {-1, 1, 0}, {1, 1, 1}});

	vao3_ = VAO::create({
		{1, vbo_pc, 6, 0, 0},
		{2, vbo_pc, 6, 3, 0},
		{3, vbo_i, 0, 0, 1} });

	GLVec3 center{0, 0, 0};
	set_scene_center(center);
	set_scene_radius(3);
}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();
	GLMat4 pmv = proj * mv;
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	prg1_->bind();
	set_uniform_value(1, pmv);
	vao1_->bind();
	glPointSize(9);
	glDrawArrays(GL_POINTS, 0, 4);

	prg2_->bind();
	set_uniform_value(1, pmv);
	vao2_->bind();
	glPointSize(5);
	glDrawArraysInstanced(GL_POINTS, 0, 4, 5);
	vao3_->bind();
	glDrawArraysInstanced(GL_LINE_LOOP, 0, 4, 5);
	
}




