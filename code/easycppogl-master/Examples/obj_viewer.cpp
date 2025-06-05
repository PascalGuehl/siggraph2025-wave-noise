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
#include<string>
#include <memory.h>
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
using namespace std::string_literals;

using namespace EZCOGL;

static const std::string phong_gen_vert = R"(
layout(location=1) uniform mat4 projectionMatrix;
layout(location=2) uniform mat4 viewMatrix;
layout(location=3) uniform mat3 normalMatrix;

layout(location=1) in vec3 position_in;
out vec3 Po;

layout(location=2) in vec3 normal_in;
out vec3 No;

#ifdef WITH_TC
layout(location=3) in vec2 texcoord_in;
out vec2 TCo;
#endif

void main()
{
	No = normalMatrix * normal_in;
	vec4 Po4 = viewMatrix * vec4(position_in,1);
	Po = Po4.xyz;
	gl_Position = projectionMatrix * Po4;
#ifdef WITH_TC
	TCo = texcoord_in;
#endif
}
)";

static const std::string phong_gen_frag = R"(

in vec3 Po;
in vec3 No;               
#ifdef WITH_TC
in vec2 TCo;
#endif

#ifdef WITH_TEX_DIFFUSE
layout(binding = 0) uniform sampler2D TU_diffuse;
#else
layout(location=11) uniform vec3 u_diffuse_color;
#endif

#ifdef WITH_TEX_AMBIENT
layout(binding = 1) uniform sampler2D TU_ambient;
#else
layout(location=12) uniform vec3 u_ambient_color;
#endif

#ifdef WITH_TEX_SPEC
layout(binding = 2) uniform sampler2D TU_specular;
#else
layout(location=13) uniform vec3 u_specular_color;
#endif

layout(location=14) uniform float u_specular_exp;

#ifdef WITH_TEX_NORMAL
layout(binding = 3) uniform sampler2D TU_normal;
in vec3 TGo;
#endif

out vec3 frag_out;

layout(location=20) uniform vec3 light_pos;



#ifdef WITH_TEX_DIFFUSE
vec3 diffuse() { return texture(TU_diffuse, TCo).rgb;}
#else
vec3 diffuse() { return u_diffuse_color;}
#endif

#ifdef WITH_TEX_AMBIENT
vec3 ambient() { return texture(TU_ambient TCo).rgb;}
#else
vec3 ambient() { return u_ambient_color;}
#endif

#ifdef WITH_TEX_SPEC
vec3 specular_col() { return texture(TU_specular TCo).rgb;}
#else
vec3 specular_col() { return u_specular_color;}
#endif

float specular_exp() { return u_specular_exp;}

#ifdef WITH_TEX_NORMAL
vec3 normal() 
{
	vec3 Tg = normalize(TGo);	
	vec3 N = normalize(No);
	mat3 TBN =  mat3(Tg,cross(N,Tg), N);
	return normalize(TBN*texture(TU_normal TCo).xyz);
}
#else
vec3 normal() { return normalize(No);}
#endif


void main()
{
	vec3 N = normal();
	vec3 L = normalize(light_pos-Po);
	float lamb = 0.9 * max(0.0,dot(N,L));

	vec3 E = normalize(-Po);
	vec3 R = reflect(-L, N);
	float spec = pow( max(dot(R,E), 0.0), specular_exp());
	vec3 col = diffuse();
	vec3 specol = specular_col();
	frag_out = mix(col*lamb,specol,spec)+0.1*ambient();
}
)";



class RenderProcessor
{
protected:
	// not responsable of deleting these objects !!
	const ShaderProgram* prg_;
	MeshRenderer::UP rend_;
public:
	RenderProcessor(const ShaderProgram* prg, MeshRenderer::UP rend) :
		prg_(prg),
		rend_(std::move(rend))
	{}

	virtual void draw(const GLMat4& proj, const GLMat4& view) = 0;
};

class RenderProcessorNoTex:public RenderProcessor
{
public:
	RenderProcessorNoTex(const ShaderProgram* prg, MeshRenderer::UP rend) :
		RenderProcessor(prg,std::move(rend))
	{}
	void draw(const GLMat4& proj, const GLMat4& view) override
	{
		prg_->bind();
		set_uniform_value(1, proj);
		set_uniform_value(2, view);
		set_uniform_value(3, Transfo::inverse_transpose(view));
		set_uniform_value(4, GLVec3(0, 1, 2));

		const Material::SP mat = rend_->material();
		set_uniform_value(11, mat->Kd);
		set_uniform_value(12, mat->Ka);
		set_uniform_value(13, mat->Ks);
		set_uniform_value(14, mat->Ns);

		rend_->draw(GL_TRIANGLES);
	}
};

class RenderProcessorTexDiffuse :public RenderProcessor
{
public:
	RenderProcessorTexDiffuse(const ShaderProgram* prg, MeshRenderer::UP rend) :
		RenderProcessor(prg,std::move(rend))
	{}
	void draw(const GLMat4& proj, const GLMat4& view) override
	{
		prg_->bind();
		set_uniform_value(1, proj);
		set_uniform_value(2, view);
		set_uniform_value(3, Transfo::inverse_transpose(view));
		set_uniform_value(20, GLVec3(0, 1, 2));
		const Material::SP mat = rend_->material();
		mat->tex_kd->bind(0);
		set_uniform_value(12, mat->Ka);
		set_uniform_value(13, mat->Ks);
		set_uniform_value(14, mat->Ns);

		rend_->draw(GL_TRIANGLES);
	}
};

class Viewer: public GLViewer
{
	 ShaderProgram::UP prg1;
	std::vector<ShaderProgram::UP> prg_phong;

	std::vector<std::unique_ptr<RenderProcessor>> renderprocs_;


	std::vector<MeshRenderer::UP> renderer_phong;
	std::vector<int> renderer_prg_;
	std::vector<MeshRenderer::UP> renderer_edges;
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
	file_dialog(nullptr),
	supported_files({"Mesh Files", "*.off *.obj *.ply", "All Files", "*" })
{}


void Viewer::init_ogl()
{
//	prg1 = ShaderProgram::create({ {GL_VERTEX_SHADER,p_vert},{GL_FRAGMENT_SHADER,p_frag} }, "pos");

	prg_phong.push_back(ShaderProgram::create({ {GL_VERTEX_SHADER,"#version 430\n"s + phong_gen_vert},{GL_FRAGMENT_SHADER,"#version 430\n"s + phong_gen_frag}}, "phong0"));
	std::string phong_ct_vert = "#version 430\n#define WITH_TC 1\n"s + phong_gen_vert;
	//int x = phong_ct_vert.find("D_1 1");
	//phong_ct_vert.replace(x, x + 9, "WITH_TC 1");

	std::string phong_diff_frag = "#version 430\n#define WITH_TC 1\n#define WITH_TEX_DIFFUSE 1\n"s + phong_gen_frag;
	prg_phong.push_back(ShaderProgram::create({ {GL_VERTEX_SHADER,phong_ct_vert},{GL_FRAGMENT_SHADER,phong_diff_frag} }, "phong_diff"));

/*	std::string phong_diff_normal_frag = phong_gen_frag;
	x = phong_diff_frag.find("D_1 1");
	phong_diff_frag.replace(x, x + 18, "WITH_TEX_DIFFUSE 1");
	x = phong_diff_frag.find("D_2 1");
	phong_diff_frag.replace(x, x + 17, "WITH_TEX_NORMAL 1");

	prg_phong.push_back(ShaderProgram::create({ {GL_VERTEX_SHADER,phong_ct_vert},{GL_FRAGMENT_SHADER,phong_diff_normal_frag} }, "phong_diff_normal"));
	*/

	auto me = Mesh::Wave(50);
	auto mat = me->material();
	mat->Ka = GLVec3(0.7f, 0.7f, 1);
	mat->Kd = GLVec3(0.9f, 0.1f, 0.1f);
	mat->Ks = GLVec3(0.9f, 0.9f, 0.9f);
	mat->Ns = 200.0f;
	mat->tex_kd = Texture2D::create();
	mat->tex_kd->load("/home/thery/Data/chaton.png");

	renderprocs_.clear();
	renderprocs_.push_back(	std::make_unique<RenderProcessorTexDiffuse>(prg_phong[0].get(), me->renderer(1, 2, 3, -1, -1)));

	set_scene_center(me->BB()->center());
	set_scene_radius(me->BB()->radius());

}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

	int ip = 0;
	for (const auto& r : renderprocs_)
	{
		r->draw(proj, mv);

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
		file_dialog = std::make_unique<pfd::open_file>("Select a mesh", ".", supported_files, true);
	ImGui::End();

	if (file_dialog && file_dialog->ready())
	{
		auto selection = file_dialog->result();
		if (!selection.empty())
		{
			auto mes = Mesh::load(selection[0]);

			BoundingBox::SP bb = BoundingBox::create();
			renderprocs_.clear();
			renderer_phong.clear();
			for (const auto& me:mes)
			{
				if (me->has_tex_coords())
				{
					if (me->material()->has_ka_texture())
					{
						renderprocs_.push_back(
							std::make_unique<RenderProcessorTexDiffuse>(prg_phong[1].get(), me->renderer(1, 2, 3, -1, -1)));
					}
				}
				else
				{
					renderprocs_.push_back(std::make_unique<RenderProcessorNoTex>(
						prg_phong[0].get(), me->renderer(1, 2, -1, -1, -1)));
				}
				bb->merge(*(me->BB()));
			}

			set_scene_center(bb->center());
			set_scene_radius(bb->radius()*2);
//			camera().look_dir(GLVec3(15, 8, 0), GLVec3(-1, 0, 0), GLVec3(0, 1, 0));
		}
		file_dialog = nullptr;
	}
}
