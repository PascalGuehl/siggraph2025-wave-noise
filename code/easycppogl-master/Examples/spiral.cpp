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
#include "ssbo.h"


#define macro_str(s) #s
#define macro_xstr(s) macro_str(s)
#define DATA_PATH std::string(macro_xstr(DEF_DATA_PATH))

using namespace EZCOGL;

static const std::string simp_vert = R"(
#version 430
layout(location=1) uniform mat4 projectionMatrix;
layout(location=2) uniform mat4 viewMatrix;
layout (std430, binding=1) buffer shader_data
{ 
  vec4 positions[2048];
  vec4 colors[2048];
};

out vec3 color;
void main()
{
	gl_Position = projectionMatrix * viewMatrix * vec4(positions[gl_VertexID]);
	color = colors[gl_VertexID].rgb;
}
)";

static const std::string simp_frag = R"(
#version 430
precision highp float;
in vec3 color;
out vec4 frag_out;
void main()
{
	frag_out = vec4(color,1);
}
)";

class Viewer: public GLViewer
{
	ShaderProgram::UP prg1;
	SSBO::UP ssbo_;

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

    prg1 = ShaderProgram::create({{GL_VERTEX_SHADER, simp_vert}, {GL_FRAGMENT_SHADER, simp_frag}}, "pos");
	ssbo_ = SSBO::create(2048*2*sizeof(GLVec4),1);

	std::vector<GLVec4> P;
	std::vector<GLVec4> C;
	P.reserve(2048);
	C.reserve(2048);
	for (int i = 0; i<2048; ++i)
	{
		float r = 1.0f - float(i) / 2248.0f;
		float a = 6.283f * float(i)/128.0f;
		P.emplace_back(r*r * std::cos(a), r*r * std::sin(a), -1.0f  + float(i)/1024, 1);
		GLVec4 col{ 0,0,0,0 };
		col[i % 3] = 1;
		C.push_back(col);
	}

	ssbo_->update(P.data(), 0, 2048 * 16);
	ssbo_->update(C.data(), 2048 * 16, 2048 * 16);

	set_scene_center(GLVec3(0, 0, 0));
	set_scene_radius(2.0f);
}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	prg1->bind();
	set_uniform_value(1,proj);
	set_uniform_value(2,mv);
	VAO::bind_none();
	glPointSize(5.0f);
	glDrawArrays(GL_POINTS,0,2048);
}

