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
#include <iostream>

using namespace EZCOGL;

static const std::string compute_src = R"(
#version 430
layout(local_size_x = 32, local_size_y = 32) in;
layout (std430, binding = 0) writeonly buffer outBuff {  vec4 vertices[]; } outBuffer;

layout (location = 1) uniform float t;
layout (location = 2) uniform float w;
layout (location = 3) uniform uint n_disp;
const float PI=3.141592654;

void main()
{
	vec2 P = vec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy)/2.0;
	vec2 XY = (vec2(gl_GlobalInvocationID.xy) - P) / P;
	float R =  length(XY);
	float H = cos(R*w*PI+t)*0.1*(1.0-R/1.42);
	uint index = n_disp*32*gl_GlobalInvocationID.y+gl_GlobalInvocationID.x;
	outBuffer.vertices[index] = vec4(XY,H,1);
}
)";

static const std::string prg1_vert = R"(
#version 430

layout(location=1) in vec4 position_in;
layout(location=1) uniform mat4 pmvMatrix;
out vec3 color;
void main()
{
	gl_Position = pmvMatrix *  position_in;
}
)";

static const std::string prg1_frag = R"(
#version 430
out vec3 frag_out;
void main()
{
	frag_out = vec3(1,1,1);
}
)";

class Viewer : public GLViewer
{
	VBO::SP vbo_p;
	VAO::UP vao1_;

	ShaderProgram::UP prg_compute;
	ShaderProgram::UP prg_vert_frag;
	ShaderProgram::UP prg_geom;
	GLuint nb_disp_ = 8u;
	GLuint nb_vert_= 0u;

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
{
}

void Viewer::init_ogl()
{
	prg_compute = ShaderProgram::create({{GL_COMPUTE_SHADER, compute_src}}, "compute1");

	prg_vert_frag = ShaderProgram::create({{GL_VERTEX_SHADER, prg1_vert}, {GL_FRAGMENT_SHADER, prg1_frag}}, "prg_vf");

	vbo_p = VBO::create(4);

	nb_vert_ = nb_disp_ * 32 * nb_disp_ * 32; 

	vbo_p->allocate(nb_vert_);

	vao1_ = VAO::create({{1, vbo_p}});
	
	set_scene_center(GLVec3{0, 0, 0});
	set_scene_radius(2);
}

void Viewer::draw_ogl()
{
	prg_compute->bind();
	set_uniform_value(1, current_time());
	set_uniform_value(2, 8.0f);
	set_uniform_value(3, nb_disp_);
	vbo_p->bind_compute(0);
	glDispatchCompute(nb_disp_, nb_disp_, 1u);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	VBO::unbind_compute(0);

	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();
	GLMat4 pmv = proj * mv;
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	prg_vert_frag->bind();
	set_uniform_value(1, pmv);
	vao1_->bind();
	glDrawArrays(GL_POINTS, 0, nb_vert_);
}
