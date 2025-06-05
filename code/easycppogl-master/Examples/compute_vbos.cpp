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
#include "texture2d.h"
#include "gl_viewer.h"



using namespace EZCOGL;

static const std::string compute_src = R"(
#version 430
layout(local_size_x = 32, local_size_y = 32) in;
layout (std430, binding = 0) writeonly buffer outBuff {  vec4 vertices[]; } outBuffer;

uniform float t;
uniform float w;
const float PI=3.141592654;

void main()
{
	vec2 P = vec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy)/2.0;
	vec2 XY = (vec2(gl_GlobalInvocationID.xy) - P) / P;
	float R =  length(XY);
	float H = cos(R*w*PI+t)*0.1*(1.0-R/1.42);
	uint index = 256*gl_GlobalInvocationID.y+gl_GlobalInvocationID.x;
	outBuffer.vertices[index] = vec4(XY,H,1);
}
)";

static const std::string p_vert = R"(
#version 430
layout(location=1) in vec4 pos;
uniform mat4 pvMatrix;
void main()
{
	gl_Position = pvMatrix*vec4(pos.xyz,1);
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


class Viewer: public GLViewer
{
	std::shared_ptr<ShaderProgram> prg_compute;
	std::shared_ptr<ShaderProgram> prg_p;
	Texture2D::SP tex_out;
	float waves;
	std::shared_ptr<VAO> vao;
	std::shared_ptr<VBO> vbo;

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
	waves(10.0)
{}


void Viewer::init_ogl()
{
	prg_compute = ShaderProgram::create({{GL_COMPUTE_SHADER,compute_src}}, "compute1");
	prg_p = ShaderProgram::create({{GL_VERTEX_SHADER,p_vert},{GL_FRAGMENT_SHADER,p_frag}}, "P");

	GLVVec4 ps;
	ps.reserve(256*256);

	for (int i= -128; i<127; i++)
	{
		float y = float(i)/128.0f;
		for (int j= -128; j<127; j++)
		{
			float x = float(j)/128.0f;
			ps.push_back(GLVec4{x,y,0.0f,1.0f});
		}
	}

	vbo = VBO::create(ps);
//	vbo = VBO::create(4);
//	vbo->allocate(256*256);

	vao = VAO::create({ {1,vbo} });

	set_scene_center(GLVec3(0,0,0));
	set_scene_radius(1.5f);
}

void Viewer::draw_ogl()
{
	prg_compute->bind();
	set_uniform_value("t", current_time());
	set_uniform_value("w", waves);
	vbo->bind_compute(0);
	glDispatchCompute(8u, 8u, 1u);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	VBO::unbind_compute(0);

	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	prg_p->bind();
	set_uniform_value("pvMatrix", proj*mv);
	set_uniform_value("color", GLVec3(1,1,0));
	vao->bind();
	glPointSize(2.0);
	glDrawArrays(GL_POINTS, 0, 256*256);
	VAO::unbind();
}


void Viewer::interface_ogl()
{
	ImGui::GetIO().FontGlobalScale = 2.0f;
	ImGui::Begin("EasyCppOGL Compute",nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::SetWindowSize({0,0});
	ImGui::Text("FPS :(%2.2lf)", fps_);
	ImGui::SliderFloat("Waves",&waves,1.0,20.0);
	ImGui::End();
}
