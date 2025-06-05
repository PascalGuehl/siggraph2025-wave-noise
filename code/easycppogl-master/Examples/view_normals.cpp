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

static const std::string N_vert = R"(
#version 430
layout(location=1) uniform mat4 projectionMatrix;
layout(location=2) uniform mat4 viewMatrix;
layout(location=4) uniform float sz;

layout(location=1) in vec3 position_in;
layout(location=2) in vec3 normal_in;

void main()
{
	vec3 P = position_in;
	if (gl_VertexID%2 != 0)
        P += sz * normalize(normal_in);
	gl_Position = projectionMatrix * viewMatrix * vec4(P,1);
}
)";

static const std::string N_frag = R"(
#version 430
precision highp float;
layout(location=5) uniform vec3 color;
out vec3 frag_out;

void main()
{
	frag_out = color;
}
)";

static const std::string OBJ_vert = R"(
#version 430
layout(location=1) uniform mat4 pvMatrix;
layout(location=1) in vec3 position_in;

void main()
{
	gl_Position = pvMatrix * vec4(position_in,1);
}
)";

static const std::string OBJ_frag = R"(
#version 430
layout(location=5) uniform vec3 color;
out vec3 frag_out;

void main()
{
	frag_out = color;
}
)";


class Viewer: public GLViewer
{
	ShaderProgram::UP prg1_;
	ShaderProgram::UP prg2_;
	std::vector <MeshRenderer::UP> rend_;
	std::vector <VAO::UP> vao_;
	std::vector <int> nbv_;
	std::shared_ptr<pfd::open_file> file_dialog;
	std::vector<std::string> supported_files;
	float length_exp_ = 0.0f;
	float length_base_;

	void load_mesh(const std::string& filename);

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
	prg1_ = ShaderProgram::create({{GL_VERTEX_SHADER, N_vert}, {GL_FRAGMENT_SHADER, N_frag}}, "normal");
	prg2_ = ShaderProgram::create({{GL_VERTEX_SHADER, OBJ_vert}, {GL_FRAGMENT_SHADER, OBJ_frag}}, "obj");

	load_mesh(DATA_PATH + "/meshes/socket.ply");
	//    auto me = Mesh::Tore(100,40,0.335);
	//auto me = Mesh::Wave(50);
	//rend_.clear();
	//rend_.push_back(me->renderer(1, -1, -1, -1, -1));
	//nbv_.clear();
	//vao_.clear();
	//nbv_.push_back(me->nb_vertices());
	//VBO::SP vbop = VBO::create(me->vertices_);
	//VBO::SP vbon = VBO::create(me->normals_);
	//vao_.push_back(VAO::create({{1, vbop, 1}, {2, vbon, 1}}));

	//length_base_ =  me->BB()->radius() /20.0f ;
	// set_scene_center(me->BB()->center());
	// set_scene_radius(me->BB()->radius());

	prg1_->bind();
	set_uniform_value(5, GLVec3{1,1,0.3f});
	ShaderProgram::unbind();

}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	glEnable(GL_DEPTH_TEST);
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, 1.1);
	prg2_->bind();
	set_uniform_value(1, proj * mv);
	set_uniform_value(5, GLVec3{0, 0, 0.3f});
	for (const auto& r : rend_)
		r->draw(GL_TRIANGLES);

	glDisable(GL_POLYGON_OFFSET_FILL);
	set_uniform_value(5, GLVec3{0, 0.5f, 0.5f});
	for (const auto& r : rend_)
		r->draw(GL_LINES);

	prg1_->bind();
	set_uniform_value(1,proj);
	set_uniform_value(2,mv);
	set_uniform_value(4, length_base_*std::pow(10.0f,length_exp_));
	auto nb_it = nbv_.begin();
	for (const auto& v : vao_)
	{
		v->bind();
		glDrawArraysInstanced(GL_LINES, 0, 2, *nb_it++);
	}
}

void Viewer::load_mesh(const std::string& filename)
{
	nbv_.clear();
	vao_.clear();
	rend_.clear();
	BoundingBox::SP bb = BoundingBox::create();
	auto mes = Mesh::load(filename);
	for (auto& me : mes)
	{
		nbv_.push_back(me->nb_vertices());
		rend_.push_back(me->renderer(1, -1, -1, -1, -1));
		VBO::SP vbop = VBO::create(me->vertices_);
		VBO::SP vbon = VBO::create(me->normals_);
		vao_.push_back(VAO::create({{1, vbop, 1}, {2, vbon, 1}}));
		bb->merge(*(me->BB()));
	}

	set_scene_center(bb->center());
	set_scene_radius(bb->radius() * 1.5f);
	length_base_ = bb->radius() / 20.0f;
}
void Viewer::interface_ogl()
{

//	std::cout << ImGui::GetDpiScale()<<std::endl;

//	ImGui::GetIO().FontGlobalScale = 1.5f;
	ImGui::Begin("Load",nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::Text("FPS: %2.2lf", fps_);
	ImGui::Text("MEM:  %2.2lf %%", 100.0 * mem_);
	ImGui::SliderFloat("length", &length_exp_,-1.0f,1.0f);

	ImGui::SetWindowSize({0,0});
	if (ImGui::Button("Load Mesh"))
		file_dialog = std::make_shared<pfd::open_file>("Select a mesh", ".", supported_files, true);
	ImGui::End();

	if (file_dialog && file_dialog->ready())
	{
		auto selection = file_dialog->result();
		if (!selection.empty())
			load_mesh(selection[0]);
		//{
		//	nbv_.clear();
		//	vao_.clear();
		//	rend_.clear();
		//	BoundingBox::SP bb = BoundingBox::create();
		//	auto mes = Mesh::load(selection[0]);
		//	for (auto& me : mes)
		//	{
		//		nbv_.push_back(me->nb_vertices());
		//		rend_.push_back(me->renderer(1, -1, -1, -1, -1));
		//		VBO::SP vbop = VBO::create(me->vertices_);
		//		VBO::SP vbon = VBO::create(me->normals_);
		//		vao_.push_back(VAO::create({ {1,vbop,1},{2,vbon,1}}));
		//		bb->merge(*(me->BB()));
		//	}

		//	set_scene_center(bb->center());
		//	set_scene_radius(bb->radius() * 1.5f);
		//	length_base_ = bb->radius() / 20.0f;

		//}
		file_dialog = nullptr;
	}
}
