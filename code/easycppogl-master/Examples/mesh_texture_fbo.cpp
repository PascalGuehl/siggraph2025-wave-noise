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
#include "fbo.h"
#include "texture2d.h"
#include "gl_viewer.h"
#include "mesh.h"

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
layout(location=7) in float posZ;

out vec3 Po;
out vec3 No;

void main()
{
	No = normalMatrix * normal_in;
	vec4 Po4 = viewMatrix * vec4(position_in+vec3(0,0,posZ),1);
	Po = Po4.xyz;
	gl_Position = projectionMatrix * Po4;
}
)";

static const std::string phong_frag = R"(
#version 430
precision highp float;
in vec3 Po;
in vec3 No;
out vec4 frag_out;

struct STR
{
 vec3 colors[2];
 float specness;
 float shininess;
};

uniform vec3 light_pos;

uniform STR mater;
void main()
{
	vec3 N = normalize(No);
	vec3 L = normalize(light_pos-Po);
	vec3 col;
	if (gl_FrontFacing==true)
	{
		col = mater.colors[0];
	}
	else
	{
		col = mater.colors[1];
		N *= -1.0;
	}

	float ps = dot(N,L);
	float lamb = clamp(ps,0.1,1.0);
	vec3 E = normalize(-Po);
	vec3 R = reflect(-L, N);
	float spec = pow( max(dot(R,E), 0.0), mater.specness);
	vec3 specol = mix(col, vec3(1.0),mater.shininess);
	frag_out = vec4(mix(col*lamb,specol,spec),1);
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
layout(location=1) uniform sampler2D TU;
layout(location=2) uniform float k;
void main()
{
	frag_out =  k*texture(TU,tc).rgb;
}
)";


class Viewer: public GLViewer
{
	ShaderProgram::UP prg1;
	ShaderProgram::UP prg_phong;
	MeshRenderer::UP renderer_phong;
	MeshRenderer::UP renderer_edges;
	InstancedMeshRenderer::UP irenderer_phong;
	InstancedMeshRenderer::UP irenderer_edges;
	FBO::SP fbo;
	ShaderProgram::UP prg_fs;
	Texture2D::SP tex;

	std::shared_ptr<pfd::open_file> file_dialog;
	std::vector<std::string> supported_files;

public:
	Viewer();
	void init_ogl() override;
	void draw_ogl() override;
	void resize_ogl(int32_t w, int32_t h) override;
	void interface_ogl() override;
};

int main(int, char**)
{
	Viewer v;
	return v.launch3d();
}

Viewer::Viewer():
	file_dialog(nullptr),
	supported_files({"Mesh Files", "*.off *.obj *.ply", "All Files", "*" })
{}


void Viewer::resize_ogl(int32_t w, int32_t h)
{
	fbo->resize(w, h);
}


void Viewer::init_ogl()
{
	prg1 = ShaderProgram::create({ {GL_VERTEX_SHADER,p_vert},{GL_FRAGMENT_SHADER,p_frag} }, "pos");
	prg_phong = ShaderProgram::create({ {GL_VERTEX_SHADER,phong_vert},{GL_FRAGMENT_SHADER,phong_frag} }, "phong");
	prg_fs = ShaderProgram::create({ {GL_VERTEX_SHADER,fs_vert},{GL_FRAGMENT_SHADER,fs_frag} }, "FullScreen");


	auto me = Mesh::Wave(20);

	renderer_phong = me->renderer(1, 2, -1, -1, -1);
	renderer_edges = me->renderer(1, -1, -1, -1, -1);

	auto vbi = VBO::create({ -0.4f, -0.2f, 0.0f, 0.2f, 0.4f }, 1);

	irenderer_phong = me->instanced_renderer({{7,vbi,1}}, 1, 2, -1, -1, -1);
	irenderer_edges = me->instanced_renderer({{7,vbi,1}}, 1, -1, -1, -1, -1);


	set_scene_center(me->BB()->center());
	set_scene_radius(me->BB()->radius());

	auto t = Texture2D::create({ GL_NEAREST, GL_CLAMP_TO_EDGE });
	t->init(GL_RGB8);
	fbo = FBO_Depth::create({ t });

	tex = Texture2D::create();
	tex->load(DATA_PATH+"/chaton.png");

}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	FBO::push();
	fbo->bind();
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	prg_fs->bind();
	set_uniform_value(2, 0.7f);
	set_uniform_value(1, tex->bind(0));
	VAO::none()->bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glEnable(GL_DEPTH_TEST);
	glClear( GL_DEPTH_BUFFER_BIT );

	prg_phong->bind();
	set_uniform_value(1,proj);
	set_uniform_value(2,mv);
	set_uniform_value(3,Transfo::inverse_transpose(mv));
	set_uniform_value("mater.colors",GLVVec3{{0,1,0},{1,0,0}});
	set_uniform_value("mater.specness",150.0f);
	set_uniform_value("mater.shininess",0.5f);
	set_uniform_value("light_pos",GLVec3(10,100,200));

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, 1.4);

	irenderer_phong->draw(GL_TRIANGLES,5);

	glDisable(GL_POLYGON_OFFSET_FILL);

	prg1->bind();
	set_uniform_value(1,proj*mv);
	set_uniform_value(2,GLVec3{1,1,0});
	irenderer_edges->draw(GL_LINES,5);


	FBO::pop();
	glDisable(GL_DEPTH_TEST);
	prg_fs->bind();
	set_uniform_value(2, 1.0f);
	set_uniform_value(1, fbo->texture(0)->bind(0));
	VAO::none()->bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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
			auto mes = Mesh::load(selection[0]);
			auto& me = mes.front();
			//renderer_edges = me->renderer(1,-1,-1,-1,-1);
			//renderer_phong = me->renderer(1,2,-1,-1,-1);

			auto vbi = VBO::create({ -2*me->BB()->radius(), -1*me->BB()->radius() , 0.0f, me->BB()->radius(), 2*me->BB()->radius()}, 1);

			irenderer_phong = me->instanced_renderer({ {7,vbi,1} }, 1, 2, -1, -1, -1);
			irenderer_edges = me->instanced_renderer({ {7,vbi,1} }, 1, -1, -1, -1, -1);

			set_scene_center(me->BB()->center());
			set_scene_radius(me->BB()->radius()*3);
		}
		file_dialog = nullptr;
	}
}
