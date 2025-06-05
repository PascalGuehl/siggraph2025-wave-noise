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

#include <shader_program.h>
#include <vbo.h>
#include <vao.h>
#include <fbo.h>
#include <texture2d.h>
#include <gl_viewer.h>
#include <mesh.h>


using namespace EZCOGL;


static const std::string imp_vert = R"(
#version 330
uniform mat4 pmvMatrix;
void main()
{
	float u = float(gl_VertexID) / 400.0;
	float a =  5.0 * u * 6.2830;
	vec2 p = u * vec2(cos(a) , sin(a));
	gl_Position = pmvMatrix*vec4(p,u/2.0,1);
}
)";

static const std::string imp_frag = R"(
#version 330
uniform vec3 color;
out vec3 frag_out;
void main()
{ 
	frag_out = color;
}
)";



static const std::string pc_vert = R"(
#version 330
layout(location=1) in vec2 position_vertex;
layout(location=2) in vec3 color_vertex;
uniform mat4 pmvMatrix;
out vec3 color;
void main()
{
	gl_Position = pmvMatrix*vec4(position_vertex,0,1);
	color = color_vertex;
}
)";

static const std::string pc_frag = R"(
#version 330
in vec3 color;
out vec3 frag_out;
void main()
{ 
	frag_out = color;
}
)";


class Viewer : public GLViewer
{
	std::shared_ptr<VAO> vao_pc;
	std::shared_ptr<ShaderProgram> prg1;
	std::shared_ptr<ShaderProgram> prg2;

	GLuint nbv_;

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
	prg1 = ShaderProgram::create({{GL_VERTEX_SHADER,imp_vert},{GL_FRAGMENT_SHADER,imp_frag}}, "pos_implicite");
	prg2 = ShaderProgram::create({{GL_VERTEX_SHADER,pc_vert},{GL_FRAGMENT_SHADER,pc_frag}}, "pos+col");

    auto vbo_p = VBO::create(GLVVec3{ {0,0,0},{0.5,0,0},{0.5,0.5,0},{0,0.5,0} });
    vbo_p->update({0.7f, 0.6f}, 6, 2); // update de 2 float en 6,7
    vbo_p->update(GLVVec3{{0, 0, 0}},0,1); // update du Vec3 en 0

	auto vbo_c = VBO::create(3);
    vbo_c->allocate(4);
    vbo_c->update({ 1.0f,0.0f,0.0f, 0.0f,1.0f,0.0f, 0.0f,0.0f,1.0f, 1.0f,1.0f,1.0f });

	vao_pc = VAO::create({ {1,vbo_p},{2,vbo_c} });
	nbv_ = vbo_p->length();
	set_scene_center(GLVec3(0,0,0));
	set_scene_radius(3.0);

}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	prg1->bind();
	set_uniform_value("pmvMatrix", proj*mv);
	set_uniform_value("color", GLVec3(1, 1, 0));
	glPointSize(2.0);
	VAO::none()->bind();
	glDrawArrays(GL_POINTS, 0, 400);

	prg2->bind();
	set_uniform_value("pmvMatrix", proj*mv*Transfo::scale(4)*Transfo::rotateZ(10*current_time())*Transfo::translate({-0.25f,-0.25f,-0.1f}));
	glPointSize(4.0);
	vao_pc->bind();
	glDrawArrays(GL_LINE_LOOP, 0, nbv_);

}




