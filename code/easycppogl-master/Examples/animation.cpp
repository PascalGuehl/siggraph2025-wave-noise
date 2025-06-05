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
#include "ebo.h"
#include "fbo.h"
#include "texture2d.h"
#include "gl_viewer.h"
#include "mesh.h"
#include "ubo.h"



using namespace EZCOGL;


static const std::string circ_vert = R"(
#version 430

layout (std140, binding=17) uniform Matrices
{
    mat4 pMatrix;
    mat4 vMatrix;
};

layout(location=1) in vec3 position_in;
layout(location=2) in float rad_in;
uniform float rad_scale;
out vec2 tc;
void main()
{
	tc = vec2(gl_VertexID%2,gl_VertexID/2)*2.0 -1.0;;
    vec4 p4 = vMatrix * vec4(position_in,1)+vec4(rad_scale*rad_in*tc,0,0);
    gl_Position = pMatrix * p4;
}
)";


static const std::string circ_frag = R"(
#version 430
uniform vec3 color;
in vec2 tc;
out vec3 frag_out;
void main()
{
	float r2 = dot(tc,tc);
	if (abs(r2-0.9)>=0.1)
		discard;
	frag_out = color;
}
)";


static const std::string disc_vert = R"(
#version 430
layout (std140, binding=17) uniform Matrices
{
    mat4 pMatrix;
    mat4 vMatrix;
};
layout(location=1) in vec3 position_in;
uniform float radius;
out vec2 tc;
void main()
{
	tc = vec2(gl_VertexID%2,gl_VertexID/2)*2.0 -1.0;;
    vec4 p4 = vMatrix * vec4(position_in,1)+vec4(radius*tc,0,0);
    gl_Position = pMatrix * p4;
}
)";

static const std::string disk_frag = R"(
#version 430
uniform vec3 color;
in vec2 tc;
out vec3 frag_out;
void main()
{
	float r2 = dot(tc,tc);
	if (r2>=1.0)
		discard;
	frag_out = color;
}
)";



static const std::string p_vert = R"(
#version 430
layout (std140, binding=17) uniform Matrices
{
    mat4 pMatrix;
    mat4 vMatrix;
};
layout (location=5) uniform mat4 modelMatrix;
layout(location=1) in vec3 position_in;
void main()
{
    gl_Position = pMatrix*vMatrix*modelMatrix*vec4(position_in,1);
}
)";

static const std::string p_frag = R"(
#version 430
uniform vec3 color;
out vec3 frag_out;
void main()
{ 
	frag_out = color;
}
)";


struct DataMatrices
{
    GLMat4 proj;
    GLMat4 view;
};


class Viewer : public GLViewer
{
	VAO::UP vao_pc;
	ShaderProgram::UP prg1;
	ShaderProgram::UP prg2;
	ShaderProgram::UP prg3;
	VBO::SP vbo_p;
	VBO::SP vbo_r;
	VAO::UP vao_c;
	VAO::UP vao_d;
	VAO::UP vao_p;
	GLuint nbv_;
	EBO::SP ebo;
	MeshRenderer::UP grid;
    UBO::UP ubo_matr;
	bool animation_on;

	std::vector<GLVec3> vertices;
	std::vector<float> rads;
	std::vector<GLuint> edges;
	GLVec3 mesh_color;
	GLVec3 circle_color;
    float circle_radius;
    void init_data();
    void animate();

    DataMatrices matrices_;

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
	animation_on(false),circle_radius(1.0f)
{}


void Viewer::init_ogl()
{
	init_data();

	prg1 = ShaderProgram::create({{GL_VERTEX_SHADER,circ_vert},{GL_FRAGMENT_SHADER,circ_frag}}, "circ");
    prg2 = ShaderProgram::create({{GL_VERTEX_SHADER,disc_vert},{GL_FRAGMENT_SHADER,disk_frag}}, "disc");
	prg3 = ShaderProgram::create({{GL_VERTEX_SHADER,p_vert},{GL_FRAGMENT_SHADER,p_frag}}, "pos");

	vbo_p = VBO::create(vertices);
	vbo_r = VBO::create(rads);
	vao_p = VAO::create({ {1,vbo_p}});
	vao_d = VAO::create({ {1,vbo_p,1} });
	vao_c = VAO::create({ {1,vbo_p,1}, {2,vbo_r,1}});
	nbv_ = vertices.size();
	ebo = EBO::create(edges);

    grid = Mesh::Grid(21,21)->renderer(1,-1,-1,-1,-1);

    ubo_matr = UBO::create(matrices_,17);
	set_scene_center(GLVec3(0,0,0));
	set_scene_radius(20);

}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();
    matrices_.proj = proj;
    matrices_.view = mv;
    ubo_matr->update(&matrices_);

	GLMat4 pv = proj*mv;

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	prg3->bind();
    GLMat4 model = Transfo::translate(GLVec3(0.f, -0.01f, 0.f)) * Transfo::rotateX(90)
                   * Transfo::scale(10);
    set_uniform_value(5, model);
	set_uniform_value("color", GLVec3(0.7f,0.7f,0.7f));
	grid->draw(GL_LINES);

    set_uniform_value(5, GLMat4::Identity());
	set_uniform_value("color", mesh_color);
	vao_p->bind();
	ebo->bind();
	glDrawElements(GL_LINES, ebo->length(), GL_UNSIGNED_INT, nullptr);

	prg2->bind();
	set_uniform_value("radius", 0.05);
	set_uniform_value("color", mesh_color);
	vao_d->bind();
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, nbv_);

	prg1->bind();
	set_uniform_value("rad_scale", 2.0f * circle_radius);
	set_uniform_value("color", circle_color);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, nbv_);

	if (animation_on)
		animate();
}

void Viewer::interface_ogl()
{
	ImGui::Begin("Animation",nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::SetWindowSize({0,0});
	ImGui::SliderFloat("Circle",&circle_radius,0.2f,2.0f);
	ImGui::Checkbox("anim",&animation_on);

	ImGui::End();
}

void Viewer::init_data()
{
	mesh_color = GLVec3(1,1,0);
	circle_color = GLVec3(0,1,1);

	vertices.reserve(8);
	vertices.emplace_back(-4, 0,-4);
	vertices.emplace_back( 4, 0,-4);
	vertices.emplace_back( 4,8,-4);
	vertices.emplace_back(-4,8,-4);
	vertices.emplace_back(-4, 0, 4);
	vertices.emplace_back( 4, 0, 4);
	vertices.emplace_back( 4,8, 4);
	vertices.emplace_back(-4,8, 4);

	float R = 0.1f;
	rads = std::vector<float>{R,0,R,0, 0,R,0,R};
	edges = std::vector<GLuint>{0,1, 1,2, 2,3, 3,0, 4,7, 7,6, 6,5, 5,4, 0,4, 1,5, 2,6, 3,7 };
}


// void Viewer::animate()
// {
// 	float t = current_time();

// 	float a = int(t/10)%2 ? 0.001f : -0.001f;
// 	float b = int(t/10)%2 ? 0.999f :  1.001f;

// 	std::vector<GLVec3> nouv(vertices.size());
// 	nouv[0] = (b *vertices[0] + a*vertices[1]);
// 	nouv[1] = (b *vertices[1] + a*vertices[5]);
// 	nouv[5] = (b *vertices[5] + a*vertices[4]);
// 	nouv[4] = (b *vertices[4] + a*vertices[0]);

// 	nouv[3] = (b *vertices[3] + a*vertices[2]);
// 	nouv[2] = (b *vertices[2] + a*vertices[6]);
// 	nouv[6] = (b *vertices[6] + a*vertices[7]);
// 	nouv[7] = (b *vertices[7] + a*vertices[3]);

// 	vertices.swap(nouv);

// 	vbo_p->update(vertices);
// }


void Viewer::animate()
{
	float t = current_time(); // en sec

	auto nouv = vertices;
	float s = 5*t;
	nouv[0] += GLVec3(0.1f*std::cos(s),0, 0.1f*std::sin(s)); s += 1.5f;
	nouv[1] += GLVec3(0.1f*std::cos(s),0, 0.1f*std::sin(s)); s += 1.5f;
	nouv[2] += GLVec3(0.1f*std::cos(s),0, 0.1f*std::sin(s)); s += 1.5f;
	nouv[3] += GLVec3(0.1f*std::cos(s),0, 0.1f*std::sin(s)); s += 1.5f;
	s = float(-5*t+M_PI);
	nouv[4] += GLVec3(0.1f*std::cos(s),0, 0.1f*std::sin(s)); s -= 1.5f;
	nouv[5] += GLVec3(0.1f*std::cos(s),0, 0.1f*std::sin(s)); s -= 1.5f;
	nouv[6] += GLVec3(0.1f*std::cos(s),0, 0.1f*std::sin(s)); s -= 1.5f;
	nouv[7] += GLVec3(0.1f*std::cos(s),0, 0.1f*std::sin(s)); s -= 1.5f;

	vbo_p->update(nouv);
}



