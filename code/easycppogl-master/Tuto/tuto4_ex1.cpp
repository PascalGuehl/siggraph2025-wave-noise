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
#define DATA_PATH std::string(macro_xstr(DDEF_DATA_PATH))
#define SHADERS_PATH std::string(macro_xstr(DDEF_SHADERS_PATH))

using namespace EZCOGL;

static const std::string p_vert = R"(
#version 430
layout(location=1) uniform mat4 projectionMatrix;
layout(location=2) uniform mat4 viewMatrix;
layout(location=3) uniform mat3 normalMatrix;

layout(location=1) in vec3 position_in;
layout(location=2) in vec3 normal_in;

layout(location=4) uniform vec3 light_pos;

out vec3 Po;
out vec3 No;
out vec3 Co;

void main()
{
	No = normalMatrix * normal_in;
	vec4 Po4 = viewMatrix * vec4(position_in,1);
	Po = Po4.xyz;
	gl_Position = projectionMatrix * Po4;

	vec3 L = normalize(light_pos-Po);
	vec3 N = normalize(No);

	float lambert = abs(dot(N,L));
	Co = vec3(1.0, 0.4, 0.4)*lambert;
}
)";

static const std::string p_frag = R"(
#version 430
in vec3 Po;
in vec3 No;
in vec3 Co;
out vec3 frag_out;

void main()
{
	vec3 N = normalize(No);
	frag_out = Co;
}
)";


// Creation du VIEWER 
class Viewer : public GLViewer
{
	ShaderProgram::UP prg_p;
	std::vector<MeshRenderer::UP> renderer_p;
	int nbMeshParts;

public:
	Viewer();
	void init_ogl() override;
	void draw_ogl() override;
	void interface_ogl() override;
};

// MAIN c'est juste creer un viewer
int main(int, char**)
{
	Viewer v;
	return v.launch3d();
}

Viewer::Viewer():nbMeshParts(0)
{
}


void Viewer::init_ogl()
{
	prg_p = ShaderProgram::create({{GL_VERTEX_SHADER, p_vert}, {GL_FRAGMENT_SHADER, p_frag}}, "prog");

	// Load OBJ file mesh
	auto mesh = Mesh::load(DATA_PATH + "/models/mustang_GT.obj");
	
	nbMeshParts = mesh.size();
	// set the renderer and the materials for all the meshes parts
	for (int i = 0; i < nbMeshParts; ++i)
	{
		renderer_p.push_back(mesh[i]->renderer(1, 2, -1, -1, -1));
	}
	set_scene_center(mesh[0]->BB()->center());
	set_scene_radius(3.0*mesh[0]->BB()->radius());
	//set_scene_radius(50.f);
}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

	prg_p->bind();

	set_uniform_value(1, proj);
	set_uniform_value(2, mv);
	set_uniform_value(3, Transfo::inverse_transpose(mv));
	set_uniform_value(4, GLVec3(0, 100, 2000));

	for (int i=0; i<renderer_p.size(); i++) renderer_p[i]->draw(GL_TRIANGLES);
}

void Viewer::interface_ogl()
{
	ImGui::Begin("Gui", nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::Text("FPS: %2.2lf", fps_);
	ImGui::Text("MEM:  %2.2lf %%", 100.0 * mem_);

	ImGui::SetWindowSize({0, 0});
	ImGui::End();
}
