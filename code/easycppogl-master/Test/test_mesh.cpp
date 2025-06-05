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

#include "fbo.h"
#include "gl_viewer.h"
#include "mesh.h"
#include "shader_program.h"
#include <iostream>

#define macro_str(s) #s
#define macro_xstr(s) macro_str(s)
#define DATA_PATH std::string(macro_xstr(DEF_DATA_PATH))

using namespace EZCOGL;

static const std::string phong_vert = R"(
#version 430
layout(location=1) uniform mat4 projectionMatrix;
layout(location=2) uniform mat4 viewMatrix;
layout(location=3) uniform mat3 normalMatrix;

layout(location=1) in vec3 position_in;
layout(location=2) in vec3 normal_in;

out vec3 Po;
out vec3 No;

void main()
{
	No = normalMatrix * normal_in;
	vec4 Po4 = viewMatrix * vec4(position_in,1);
	Po = Po4.xyz;
	gl_Position = projectionMatrix * Po4;
}
)";

static const std::string phong_frag = R"(
#version 430
precision highp float;
in vec3 Po;
in vec3 No;
out vec3 frag_out;

layout(location=10) uniform vec3 color_diff;
layout(location=11) uniform vec3 color_amb;
layout(location=12) uniform vec3 color_spec;
layout(location=13) uniform float Ns;

layout(location=14) uniform vec3 light_pos;

void main()
{
	vec3 N = normalize(No);
	vec3 L = normalize(light_pos-Po);
	vec3 col;
	//if (gl_FrontFacing==true)
	//{
	//	col = mater.colors[0];
	//}
	//else
	//{
	//	col = mater.colors[1];
	//	N *= -1.0;
	//}

	if (gl_FrontFacing==false)
		N = -N;
	float lamb = max(0.0,dot(N,L));
	vec3 E = normalize(-Po);
	vec3 R = reflect(-L, N);
	float spec = Ns==0.0 ? 0.0 : pow( max(dot(R,E), 0.0), Ns);
	frag_out = mix(color_amb+color_diff*lamb,color_spec,spec);
//	frag_out = min(color_amb+color_diff*lamb+color_spec*spec,vec3(1));
//	frag_out = abs(N) + 0.000001*vec3(lamb);
}
)";

static const std::string p_vert = R"(
#version 430
layout(location=1) in vec3 pos;
layout(location=7) in float posZ;
layout(location=1) uniform mat4 pvMatrix;

void main()
{
	gl_Position = pvMatrix*vec4(pos+vec3(0,0,posZ),1);
}
)";

static const std::string p_frag = R"(
#version 430
precision highp float;
layout(location=2) uniform vec3 color;
out vec3 frag_out;
void main()
{
	frag_out = color;
}
)";

class Viewer : public GLViewer
{
	ShaderProgram::UP prg1;
	ShaderProgram::UP prg_phong;
	MeshRenderer::UP renderer_phong;
	MeshRenderer::UP renderer_edges;

	std::shared_ptr<pfd::open_file> file_dialog;
	std::vector<std::string> supported_files;

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

Viewer::Viewer() : file_dialog(nullptr), supported_files({"Mesh Files", "*.off *.obj *.ply", "All Files", "*"})
{
}


void Viewer::init_ogl()
{
	prg1 = ShaderProgram::create({{GL_VERTEX_SHADER, p_vert}, {GL_FRAGMENT_SHADER, p_frag}}, "pos");
	prg_phong = ShaderProgram::create({{GL_VERTEX_SHADER, phong_vert}, {GL_FRAGMENT_SHADER, phong_frag}}, "phong");

	auto me = Mesh::Wave(60);

	renderer_phong = me->renderer(1, 2, -1, -1, -1);
	renderer_edges = me->renderer(1, -1, -1, -1, -1);

	auto vbi = VBO::create({-0.4f, -0.2f, 0.0f, 0.2f, 0.4f}, 1);

	set_scene_center(me->BB()->center());
	set_scene_radius(me->BB()->radius());

}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1, 0.1, 0.2, 0.0);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	/*
	prg_phong->bind();
	auto mat = renderer_phong->material();
	set_uniform_value(1, proj);
	set_uniform_value(2, mv);
	set_uniform_value(3, Transfo::inverse_transpose(mv));

	set_uniform_value(10, mat->Kd);
	set_uniform_value(11, mat->Ka);
	set_uniform_value(12, mat->Ks);
	set_uniform_value(13, mat->Ns);
	set_uniform_value("light_pos", GLVec3(0, 100, 2000));

	//glEnable(GL_POLYGON_OFFSET_FILL);
	//glPolygonOffset(1.0, 1.4);

	renderer_phong->draw(GL_TRIANGLES);
	*/
	//glDisable(GL_POLYGON_OFFSET_FILL);

	prg1->bind();
	set_uniform_value(1, proj * mv);
	set_uniform_value(2, GLVec3{1, 1, 0});
	renderer_edges->draw(GL_LINES);

}

void Viewer::interface_ogl()
{
	ImGui::Begin("Load", nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::Text("FPS: %2.2lf", fps_);
	ImGui::Text("MEM:  %2.2lf %%", 100.0 * mem_);

	ImGui::SetWindowSize({0, 0});
	if (ImGui::Button("Load Mesh"))
		file_dialog = std::make_shared<pfd::open_file>("Select a mesh", ".", supported_files, true);
	ImGui::End();

	if (file_dialog && file_dialog->ready())
	{
		auto selection = file_dialog->result();
		if (!selection.empty())
		{
			auto mes = Mesh::load(selection[0]);
			auto& me = mes.front();
			renderer_edges = me->renderer(1,-1,-1,-1,-1);
			renderer_phong = me->renderer(1,2,-1,-1,-1);
			set_scene_center(me->BB()->center());
			set_scene_radius(me->BB()->radius());
		}
		if (selection.size() > 1)
			std::cerr << "Warning multiple part object, only first loaded" << std::endl;
		file_dialog = nullptr;
	}
}
