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
#include "mesh.h"
#include "gl_viewer.h"

#define macro_str(s) #s
#define macro_xstr(s) macro_str(s)
#define DATA_PATH std::string(macro_xstr(DEF_DATA_PATH))

using namespace EZCOGL;

static const std::string TBN_vert = R"(
#version 430
layout(location=1) uniform mat4 projectionMatrix;
layout(location=2) uniform mat4 viewMatrix;
layout(location=4) uniform float sz;

layout(location=1) in vec3 position_in;
layout(location=2) in vec3 normal_in;
layout(location=3) in vec3 tangent_in;

flat out vec3 color;
void main()
{
	vec3 P = position_in;
	uint vid = uint(gl_VertexID);
	uint vvid = (vid/2u)%3u;
	if (vid%2 != 0)
	{
		switch(vvid)
		{
        case 0: P += sz * normalize(normal_in);
				break;
        case 1: P += sz * normalize(tangent_in);
				break;
		case 2: P += sz * cross(normal_in,tangent_in);
				break;
		}
	}
	gl_Position = projectionMatrix * viewMatrix * vec4(P,1);
	color = vec3(0);
	color[vvid] = 1;
}
)";

static const std::string TBN_frag = R"(
#version 430
precision highp float;
flat in vec3 color;
out vec4 frag_out;

void main()
{
	frag_out = vec4(color,1);
}
)";


class Viewer: public GLViewer
{
	ShaderProgram::UP prg1;
	std::vector <VAO::UP> vao_;
	std::vector <int> nbv_;
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

Viewer::Viewer():
	nbv_(0),
	file_dialog(nullptr),
	supported_files({"Mesh Files", "*.off *.obj *.ply", "All Files", "*" })
{}


void Viewer::init_ogl()
{
	prg1 = ShaderProgram::create({ {GL_VERTEX_SHADER,TBN_vert},{GL_FRAGMENT_SHADER,TBN_frag} }, "tbn");


//    auto me = Mesh::Tore(100,40,0.335);
    auto me = Mesh::Wave(100);
	nbv_.clear();
	vao_.clear();
	nbv_.push_back(me->nb_vertices());
	VBO::SP vbop = VBO::create(me->vertices_);
	VBO::SP vbon = VBO::create(me->normals_);
	VBO::SP vbot = VBO::create(me->tangents_);
	vao_.push_back(VAO::create({ {1,vbop,1},{2,vbon,1},{3,vbot,1} }));

	prg1->bind();
    set_uniform_value(4,me->BB()->radius()/100.0f );
	prg1->unbind();

	set_scene_center(me->BB()->center());
	set_scene_radius(me->BB()->radius());
	std::cout << me->BB()->min() << std::endl;
	std::cout << me->BB()->max() << std::endl;

}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	glEnable(GL_DEPTH_TEST);
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	prg1->bind();
	set_uniform_value(1,proj);
	set_uniform_value(2,mv);
	auto nb_it = nbv_.begin();
	for (const auto& v : vao_)
	{
		v->bind();
		glDrawArraysInstanced(GL_LINES, 0, 6, *nb_it++);
	}
}

void Viewer::interface_ogl()
{

//	std::cout << ImGui::GetDpiScale()<<std::endl;

//	ImGui::GetIO().FontGlobalScale = 1.5f;
	ImGui::Begin("Load",nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::Text("FPS: %2.2lf", fps_);
	ImGui::Text("MEM:  %2.2lf %%", 100.0 * mem_);

	ImGui::SetWindowSize({0,0});
	if (ImGui::Button("Load Mesh"))
		file_dialog = std::make_shared<pfd::open_file>("Select a mesh", ".", supported_files, true);
	ImGui::End();

	if (file_dialog && file_dialog->ready())
	{
		auto selection = file_dialog->result();
		if (!selection.empty())
		{
			nbv_.clear();
			vao_.clear();
			BoundingBox::SP bb = BoundingBox::create();
			auto mes = Mesh::load(selection[0]);
			for (auto& me : mes)
			{
				nbv_.push_back(me->nb_vertices());

				VBO::SP vbop = VBO::create(me->vertices_);
				VBO::SP vbon = VBO::create(me->normals_);
				VBO::SP vbot = VBO::create(me->tangents_);
				if (vbot->length() == vbop->length())
				{
					vao_.push_back(VAO::create({ {1,vbop,1},{2,vbon,1},{3,vbot,1} }));
				}
				bb->merge(*(me->BB()));
			}

			prg1->bind();
			set_scene_center(bb->center());
			set_scene_radius(bb->radius() * 3);

		}
		file_dialog = nullptr;
	}
}
