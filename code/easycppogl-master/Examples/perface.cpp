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


#include <random>
#include <iostream>
#include "shader_program.h"
#include "vbo.h"
#include "vao.h"
#include "fbo.h"
#include "transform_feedback.h"
#include "texturebuffer.h"
#include "gl_viewer.h"
#include "mesh.h"

#define macro_str(s) #s
#define macro_xstr(s) macro_str(s)
#define DATA_PATH std::string(macro_xstr(DEF_DATA_PATH))

using namespace EZCOGL;

//class NormalCompute


static const std::string compute_normal_vert =
R"(
#version 430
layout(location=3) uniform usamplerBuffer tri_indices;
layout(location=4) uniform samplerBuffer position_vertex;
layout(location=5) uniform ivec2 imgSz;

out vec3 No;

void main()
{
	int t = 3*gl_InstanceID;
	int iiA = t + gl_VertexID;
	int iiB = t + (gl_VertexID+1)%3;
	int iiC = t + (gl_VertexID+2)%3;
	int iA = int(texelFetch(tri_indices,iiA).r);
	int iB = int(texelFetch(tri_indices,iiB).r);
	int iC = int(texelFetch(tri_indices,iiC).r);
	vec3 A = texelFetch(position_vertex, iA).rgb;
	vec3 B = texelFetch(position_vertex, iB).rgb;
	vec3 C = texelFetch(position_vertex, iC).rgb;
	No = cross (B-A, C-A);
	gl_Position = vec4(
		-1.0 +(2.0*float(iA%imgSz.x)+1.0)/float(imgSz.x),
		-1.0 +(2.0*float(iA/imgSz.x)+1.0)/float(imgSz.y),
		0.0,1.0);
	}
)";

static const std::string compute_normal_frag =
R"(
#version 430
in vec3 No;
out vec3 frag_out;
void main()
{
	frag_out = No;
}
)";

static const std::string tfb_normal_vert =
R"(
#version 430
layout(location=1) uniform sampler2D TUn;

out vec3 normal_out;

void main()
{
	int w = textureSize(TUn,0).x;
	ivec2 icoord = ivec2(gl_VertexID%w,gl_VertexID/w);
	normal_out = normalize(texelFetch(TUn,icoord,0).rgb);
}
)";

static const std::string phong_vert =
R"(
#version 430
layout(location=0) uniform mat4 projectionMatrix;
layout(location=1) uniform mat4 viewMatrix;
layout(location=2) uniform mat3 normalMatrix;

layout(location=3) uniform usamplerBuffer tri_indices;
layout(location=4) uniform samplerBuffer position_vertex;
layout(location=5) uniform samplerBuffer normal_vertex;
layout(location=6) uniform samplerBuffer color_face;
layout(location=7) uniform usamplerBuffer embedding;


out vec3 Po;
out vec3 No;
flat out vec3 Co;
void main()
{
	int ind_v = int(texelFetch(tri_indices,3*gl_InstanceID+gl_VertexID).r);
	vec3 normal_in = texelFetch(normal_vertex, ind_v).rgb;
	vec3 position_in = texelFetch(position_vertex, ind_v).rgb;
	int emb = int(texelFetch(embedding, gl_InstanceID).r);
	Co = texelFetch(color_face, emb).rgb;;

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
in flat vec3 Co;

out vec3 frag_out;

layout(location=10) uniform float specness;
layout(location=11) uniform float shininess;
layout(location=12) uniform vec3 light_pos;

void main()
{
	vec3 N = normalize(No);
	vec3 L = normalize(light_pos-Po);
	if (gl_FrontFacing==false)
		N *= -1.0;

	float lamb = 0.2+0.8*max(0.0,dot(N,L));
	vec3 E = normalize(-Po);
	vec3 R = reflect(-L, N);
	float spec = pow( max(dot(R,E), 0.0), specness);
	vec3 specol = mix(Co, vec3(1.0),shininess);
	frag_out = mix(Co*lamb,specol,spec);
}
)";


static const std::string edges_vert =
R"(
#version 430
layout(location=0) uniform mat4 projectionMatrix;
layout(location=1) uniform mat4 viewMatrix;

layout(location=3) uniform usamplerBuffer line_indices;
layout(location=4) uniform samplerBuffer position_vertex;
layout(location=6) uniform samplerBuffer color_edge;

flat out vec3 Co;
void main()
{
	int ind_v = int(texelFetch(line_indices,2*gl_InstanceID+gl_VertexID).r);
	vec3 position_in = texelFetch(position_vertex, ind_v).rgb;
	Co = texelFetch(color_edge, gl_InstanceID).rgb;

	vec4 Po4 = viewMatrix * vec4(position_in,1);
	gl_Position = projectionMatrix * Po4;
}
)";

static const std::string edges_frag = R"(
#version 430
in flat vec3 Co;
out vec3 frag_out;
void main()
{
	frag_out = Co;
}
)";


static const std::string fs_vert =R"(
#version 330
out vec2 tc;
void main()
{
	tc = vec2(gl_VertexID%2,gl_VertexID/2);
	gl_Position = vec4(2.0*tc-1.0,0,1);
}
)";

static const std::string fs_frag = R"(
#version 330
out vec3 frag_out;
in vec2 tc;
uniform sampler2D TU;
void main()
{
	frag_out = normalize(texture(TU,tc).rgb);
}
)";


class Viewer: public GLViewer
{
	ShaderProgram::UP prg_fs;
	ShaderProgram::UP prg_phong;
	ShaderProgram::UP prg_edges;
	ShaderProgram::UP prg_compute_normal;

	EBO::SP ebo_tris;
	VBO::SP vbo_vert;
	VBO::SP vbo_normal;
	VBO::SP vbo_color;
	FBO::SP fbo_normal;
	TransformFeedback::UP trsffb;

	SP_TextureUIBuffer tb_tris;
	SP_TextureBuffer tb_vert;
	SP_TextureBuffer tb_normal;
	SP_TextureBuffer tb_color_face;
	SP_TextureUIBuffer tb_embf;
	SP_TextureUIBuffer tb_lines;
	SP_TextureBuffer tb_color_edge;

	uint32_t nb_tris;
	uint32_t nb_lines;
	uint32_t nb_vert;

	static const int WIDTH_TEX_VBO = 4096;

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

Viewer::Viewer()
{}

void Viewer::init_ogl()
{
	prg_compute_normal = ShaderProgram::create({{GL_VERTEX_SHADER,compute_normal_vert},{GL_FRAGMENT_SHADER,compute_normal_frag}}, "compute_normal");
	prg_phong = ShaderProgram::create({{GL_VERTEX_SHADER,phong_vert},{GL_FRAGMENT_SHADER,phong_frag}}, "phong");
	prg_edges = ShaderProgram::create({{GL_VERTEX_SHADER,edges_vert},{GL_FRAGMENT_SHADER,edges_frag}}, "edge");
	prg_fs = ShaderProgram::create({{GL_VERTEX_SHADER,fs_vert},{GL_FRAGMENT_SHADER,fs_frag}}, "fs");

	auto me = Mesh::Wave(400);

{
	auto start = std::chrono::system_clock::now();
	me->compute_normals();
	auto end = std::chrono::system_clock::now();
	std::chrono::duration<float> elapsed_seconds = end-start;
	std::cout << "normals CPU: " << elapsed_seconds.count() << std::endl;
}

	nb_vert = me->vertices_.size();

	set_scene_center(me->BB()->center());
	set_scene_radius(me->BB()->radius());

	vbo_vert = VBO::create(me->vertices_);
	tb_vert = TextureBuffer::create(vbo_vert);
	vbo_normal = VBO::create(me->normals_);
	tb_normal = TextureBuffer::create(vbo_normal);
	ebo_tris = EBO::create(me->tri_indices);
	tb_tris = TextureUIBuffer::create(ebo_tris);
	tb_lines = TextureUIBuffer::create(EBO::create(me->line_indices));

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0.5f,1.0f);

	int nb_col = 20;
	GLVVec3 colors(nb_col);
	for (GLVec3& c: colors)
	{
		c.x() = dis(gen);
		c.y() = dis(gen);
		c.z() = dis(gen);
	}
	tb_color_face = TextureBuffer::create(VBO::create(colors));

	std::uniform_int_distribution<uint32_t> idis(0,19);

	nb_tris = me->tri_indices.size()/3u;
	std::vector<GLuint> emb(nb_tris);
	for (int i=0;i<nb_tris; i += 2)
	{
		emb[i] = idis(gen);
		emb[i+1] = emb[i];
	}
	tb_embf = TextureUIBuffer::create(EBO::create(emb));


	nb_lines = me->line_indices.size()/2u;
	colors.resize(nb_lines);
	for (GLVec3& c: colors)
	{
		c.x() = 0;
		c.y() = 0;
		c.z() = 0;
	}
	tb_color_edge = TextureBuffer::create(VBO::create(colors));


	auto tex = Texture2D::create();
	tex->init(GL_RGB32F);
	fbo_normal = FBO::create({tex});
	fbo_normal->resize(WIDTH_TEX_VBO,(me->vertices_.size()+WIDTH_TEX_VBO-1)/WIDTH_TEX_VBO);
	std::vector<std::pair<GLenum,const std::string&>> src = {{GL_VERTEX_SHADER,tfb_normal_vert}};
	std::vector<std::string> outs = {"normal_out"};
	trsffb = TransformFeedback::create(src, outs, "tfb_normal");

	{
	auto start = std::chrono::system_clock::now();
	FBO::push();
	fbo_normal->bind();
	glDisable(GL_DEPTH_TEST);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
	prg_compute_normal->bind();
	set_uniform_value(3,tb_tris->bind(0));
	set_uniform_value(4,tb_vert->bind(1));
	int nb = tb_vert->vbo().length();
	set_uniform_value(5,std::array<int32_t,2>{WIDTH_TEX_VBO,(nb+WIDTH_TEX_VBO-1)/WIDTH_TEX_VBO});
	VAO::none()->bind();
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE,GL_ONE);
	glPointSize(1.0f);
	glDrawArraysInstanced(GL_POINTS,0,3,nb_tris);
	glDisable(GL_BLEND);
	FBO::pop();

	trsffb->start(GL_POINTS,{vbo_normal});
	set_uniform_value(1,fbo_normal->texture(0)->bind(0));
	VAO::none()->bind();
	glDrawArrays(GL_POINTS,0,nb_vert);
	trsffb->stop();

	glFlush();

	auto end = std::chrono::system_clock::now();
	std::chrono::duration<float>  elapsed_seconds = end-start;
	std::cout << "normals GPU: " << elapsed_seconds.count() << std::endl;
	}




}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();


	glEnable(GL_DEPTH_TEST);
//	glDepthFunc(GL_LESS);
	glClear( GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4,4);
	prg_phong->bind();
	set_uniform_value(0,proj);
	set_uniform_value(1,mv);
	set_uniform_value(2,Transfo::inverse_transpose(mv));
	set_uniform_value(3,tb_tris->bind(0));
	set_uniform_value(4,tb_vert->bind(1));
	set_uniform_value(5,tb_normal->bind(2));
	set_uniform_value(6,tb_color_face->bind(3));
	set_uniform_value(7,tb_embf->bind(4));
	set_uniform_value(10,150.0f);
	set_uniform_value(11,0.5f);
	set_uniform_value(12,GLVec3(10,100,200));
	VAO::none()->bind();
	glDrawArraysInstanced(GL_TRIANGLES,0,3,nb_tris);

//	glDisable(GL_POLYGON_OFFSET_FILL);
////	glDepthFunc(GL_LEQUAL);
//	prg_edges->bind();
//	set_uniform_value(0,proj);
//	set_uniform_value(1,mv);
//	set_uniform_value(3,tb_lines->bind(0));
//	set_uniform_value(4,tb_vert->bind(1));
//	set_uniform_value(6,tb_color_edge->bind(3));
//	VAO::none()->bind();
//	glDrawArraysInstanced(GL_LINES,0,2,nb_lines);

}

void Viewer::interface_ogl()
{
	ImGui::GetIO().FontGlobalScale = 2.0f;
	ImGui::Begin("Texture buffer",nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::SetWindowSize({0,0});
	ImGui::Text("FPS :(%2.2lf)", fps_);
	ImGui::End();
}
