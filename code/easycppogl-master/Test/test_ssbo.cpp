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
layout (std430, binding=1) buffer shader_data
{
  vec4 positions[2048];
  vec4 colors[2048];
  mat4 projectionMatrix;
  mat4 viewMatrix;
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

class Viewer : public GLViewer
{
	ShaderProgram::UP prg1_;
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
{
}


void Viewer::init_ogl()
{

	prg1_ = ShaderProgram::create({{GL_VERTEX_SHADER, simp_vert}, {GL_FRAGMENT_SHADER, simp_frag}}, "pos");

	// warning for aligment reason, it does not work with vec3
	ssbo_ = SSBO::create(2048 * 2 * sizeof(GLVec4) + 2 * sizeof(GLMat4), 1);

	std::vector<GLVec4> P;
	std::vector<GLVec4> C;
	P.reserve(2048);
	C.reserve(2048);
	for (int i = 0; i < 2048; ++i)
	{
		float r = 1.0f - float(i) / 2248.0f;
		float a = 6.283f * float(i) / 128.0f;
		P.emplace_back(r * r * std::cos(a), r * r * std::sin(a), -1.0f + float(i) / 1024, 1);
		GLVec4 col{0, 0, 0, 0};
		col[i % 3] = 1;
		C.push_back(col);
	}

	// should certainl be better to interleave, but let's begin easy
	ssbo_->update(P.data(), 0, 2048 * 16); // for P
	ssbo_->update(C.data(), 2048 * 16, 2048 * 16); // for C

	set_scene_center(GLVec3(0, 0, 0));
	set_scene_radius(2.0f);
}

void Viewer::draw_ogl()
{
	GLMat4 matrices[2];
	matrices[0] = this->get_projection_matrix();
	matrices[1] = this->get_view_matrix();
	// update only the two matrices at the end
	ssbo_->update(matrices[0].data(), 2048 * 2 * sizeof(GLVec4), 2 * sizeof(GLMat4));

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	prg1_->bind();
	VAO::bind_none(); // all i in the SSBO
	glPointSize(5.0f);
	glDrawArrays(GL_LINE_STRIP,0,2048);
}
