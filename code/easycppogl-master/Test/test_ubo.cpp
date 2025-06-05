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

#include "gl_viewer.h"
#include "shader_program.h"
#include "ubo.h"
#include <iostream>

using namespace EZCOGL;

static const std::string pr1_vert = R"(
#version 430
layout (std140, binding=7) uniform Matrices
{
    mat4 pMatrix;
    mat4 vMatrix;
    vec3 color1;
    vec3 color2;
};

layout (location = 1) uniform int nb;
out vec3 col;
void main()
{
    float a = 6.283*float(gl_VertexID)/float(nb);
    gl_Position = pMatrix*vMatrix*vec4(0.5*cos(a),0.5*sin(a),0,1);
    col = mix(color1,color2,(0.5+0.5+sin(4.0*a)));
}
)";

static const std::string pr1_frag = R"(
#version 430
in vec3 col;
out vec3 frag_out;
void main()
{
        frag_out = col;
}
)";

static const std::string pr2_vert = R"(
#version 430
layout (std140, binding=7) uniform Matrices
{
    mat4 pMatrix;
    mat4 vMatrix;
    vec3 color1;
    vec3 color2;
};

layout (location = 1) uniform int nb;
out vec3 col;
void main()
{
    float a = 6.283*float(gl_VertexID)/float(nb);
    gl_Position = pMatrix*vMatrix*vec4(0.75*cos(a),0.75*sin(a),0,1);
    col = mix(color1,color2,(0.5+0.5+sin(2.0*a)));
}
)";

static const std::string pr2_frag = R"(
#version 430
in vec3 col;
out vec3 frag_out;
void main()
{
        frag_out = col;
}
)";

struct UBOData
{
	GLMat4 proj;
	GLMat4 view;
	GLVec3 color1;
	float pad_1; // need padding for vec alignment on 16 bytes multiple
	GLVec3 color2;
};

class Viewer : public GLViewer
{
	ShaderProgram::UP prg1;
	ShaderProgram::UP prg2;
	int nb_;
	UBO::UP ubo_;
	UBOData ubo_data_;

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

Viewer::Viewer() : nb_(255)
{
}

void Viewer::init_ogl()
{
	prg1 = ShaderProgram::create({{GL_VERTEX_SHADER, pr1_vert}, {GL_FRAGMENT_SHADER, pr1_frag}}, "prg");
	prg2 = ShaderProgram::create({{GL_VERTEX_SHADER, pr2_vert}, {GL_FRAGMENT_SHADER, pr2_frag}}, "prg2");
	ubo_ = UBO::create(ubo_data_, 7);
	set_scene_center(GLVec3(0, 0, 0));
	set_scene_radius(1);
}

void Viewer::draw_ogl()
{
	glDisable(GL_DEPTH_TEST);
	glClearColor(0.1, 0.1, 0.1, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	//  ubo_data_.color = GLVec3(1, 0, 1);
	ubo_data_.proj = proj;
	ubo_data_.view = mv;
	ubo_data_.color1 = GLVec3(1, 1, 0);
	ubo_data_.color2 = GLVec3(0, 1, 1);
	ubo_->update(&ubo_data_);

	VAO::bind_none();
	prg1->bind();
	set_uniform_value(1, 127);
	glDrawArrays(GL_LINE_STRIP, 0, 128);

	prg2->bind();
	set_uniform_value(1, 63);
	glDrawArrays(GL_LINE_STRIP, 0, 64);
}
