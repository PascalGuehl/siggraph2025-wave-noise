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
#include "texture2d.h"
#include "gl_viewer.h"



using namespace EZCOGL;

static const std::string compute_src = R"(
#version 430
layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba32f, binding = 0) uniform image2D img_output;
uniform float t;
uniform float w;
const float PI=3.141592654;

void main()
{
	vec2 R = vec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy)/2.0;
	vec2 RL = (vec2(gl_GlobalInvocationID.xy) - R)/R;
	float V = cos(length(RL)*w*PI + t)*0.5+0.5;

	float k = float((gl_WorkGroupID.x+gl_WorkGroupID.y)%2);

	vec2 r = vec2(gl_WorkGroupSize.xy)/2.0;
	vec2 rl = (vec2(gl_LocalInvocationID.xy) - r)/r;
	float v = cos(length(rl)*4.0*PI +(k*2.0-1.0)*t)*0.5+0.5;

   vec2 col = mix(vec2(1,0),vec2(0,1),k);

	vec4 pixel = vec4(v*vec3(V*col,0),1);
	imageStore(img_output, ivec2(gl_GlobalInvocationID.xy), pixel);
}
)";

static const std::string fs_vert =R"(
#version 430
out vec2 tc;
void main()
{
	tc = vec2(gl_VertexID%2,gl_VertexID/2);
	gl_Position = vec4(2.0*tc-1.0,0,1);
}
)";

static const std::string fs_frag = R"(
#version 430
out vec3 frag_out;
in vec2 tc;
uniform sampler2D TU;
void main()
{
	frag_out = texture(TU,tc).rgb;
}
)";


class Viewer: public GLViewer
{
	std::shared_ptr<ShaderProgram> prg_compute;
	std::shared_ptr<ShaderProgram> prg_fs;
	Texture2D::SP tex;
	float waves_;
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
	waves_(10.0)
{}


void Viewer::init_ogl()
{
	prg_compute = ShaderProgram::create({{GL_COMPUTE_SHADER,compute_src}}, "compute1");
	prg_fs = ShaderProgram::create({{GL_VERTEX_SHADER,fs_vert},{GL_FRAGMENT_SHADER,fs_frag}}, "FullScreen");

	tex = Texture2D::create();
	tex->alloc(512,512,GL_RGBA32F);
}

void Viewer::draw_ogl()
{
	prg_compute->bind();
	set_uniform_value("t", current_time());
	set_uniform_value("w", waves_);
	tex->bind_compute_out(0);
	glDispatchCompute(GLuint(tex->width()/32), GLuint(tex->height()/32), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glDisable(GL_DEPTH_TEST);
	prg_fs->bind();
	set_uniform_value("TU", tex->bind(0));
	VAO::none()->bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	Texture2D::unbind();
}


void Viewer::interface_ogl()
{
	ImGui::GetIO().FontGlobalScale = 2.0f;
	ImGui::Begin("EasyCppOGL Compute",nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::SetWindowSize({0,0});
	ImGui::Text("FPS :(%2.2lf)", fps_);
	ImGui::SliderFloat("Patches",&waves_,1.0,20.0);
	ImGui::End();
}
