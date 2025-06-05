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
#include "gl_viewer.h"


#define macro_str(s) #s
#define macro_xstr(s) macro_str(s)
#define DATA_PATH std::string(macro_xstr(DEF_DATA_PATH))


using namespace EZCOGL;

static const std::string compute_normal = R"(
#version 430
layout(local_size_x = 32, local_size_y = 32) in;

layout(r32f, binding = 0) uniform image2D img_in;
layout(rgba32f, binding = 1) uniform image2D img_out;

void main()
{
	ivec2 t = ivec2(gl_GlobalInvocationID.xy);
	float dhdx = (imageLoad(img_in,t+ivec2(1,0)).r - imageLoad(img_in,t-ivec2(1,0)).r);
	float dhdy = (imageLoad(img_in,t+ivec2(0,1)).r - imageLoad(img_in,t-ivec2(0,1)).r);
	vec2 k = vec2(2)/imageSize(img_in).xy;
	vec3 U = vec3(k.x,0,dhdx);
	vec3 V = vec3(0,k.y,dhdy);
	vec4 N = vec4(normalize(cross(U,V)),1);// *0.00001 + vec4(0,0.9,0.5,1);
	imageStore(img_out, t, N);
}
)";

static const std::string compute_sub = R"(
#version 430
layout(local_size_x = 32, local_size_y = 32) in;
layout (location=0) uniform int sub;
layout(r32f, binding = 0) uniform image2D img_in;
layout(r32f, binding = 1) uniform image2D img_out;

void main()
{
	float v = 0.0;
	ivec2 t = sub*ivec2(gl_GlobalInvocationID.xy);
	float k = 1.0/float(sub*sub);
	for (int j=0; j<sub; ++j)
		for (int i=0; i<sub; ++i)
			v += imageLoad(img_in,t+ivec2(i,j)).r * k;
	imageStore(img_out, ivec2(gl_GlobalInvocationID.xy), vec4(v,0,0,0));
}
)";


static const std::string compute_curv = R"(
#version 430
layout(local_size_x = 32, local_size_y = 32) in;

layout(r32f, binding = 0) uniform image2D img_in;
layout(r32f, binding = 1) uniform image2D img_out;

void main()
{
	ivec2 t = ivec2(gl_GlobalInvocationID.xy);
	float dhdx = imageLoad(img_in,t+ivec2(1,0)).r - imageLoad(img_in,t-ivec2(1,0)).r;
	float dhdy = imageLoad(img_in,t+ivec2(0,1)).r - imageLoad(img_in,t-ivec2(0,1)).r;
	imageStore(img_out, t, vec4(max(abs(dhdx),abs(dhdy)),0,0,0));
}
)";


static const std::string disk_vert =R"(
#version 430
layout (location=0) in vec2 pos;
uniform float width;
out vec2 lc;
void main()
{
	uint id = uint(gl_VertexID);
	lc = vec2(id%2u,id/2u)*2.0 -1.0;
	gl_Position = vec4(lc*width + pos,0,1);
}
)";

static const std::string disk_frag =R"(
#version 430
in vec2 lc;
out vec4 frag_out;

void main()
{
	float l = dot(lc,lc);
	if (l>1.0)
		discard;
	float ml = 1.0-l;
	frag_out = vec4(smoothstep(0.0,1.0,ml),0,0,ml);
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
//	float v = texture(TU,tc).r;
//	frag_out = vec3(v,v,v);
	frag_out = abs(texture(TU,tc).rgb);
}
)";







static const std::string tess_vert = R"(
#version 430
layout(location=0) uniform mat4 vMatrix;
layout(location=3) uniform int nb;
layout(location=6) uniform float adapt;
layout(binding=2) uniform sampler2D TUc;
out float d;
void main()
{
	int idv = gl_VertexID%4;
	int idp = gl_VertexID/4;
	int xi = idp%nb;
	int yi = idp/nb;
	vec2 Pa = (vec2(xi,yi)+0.5)/float(nb);
	vec2 P = (vec2(xi,yi)+vec2(((idv+1)%4)/2,idv/2))/float(nb);
	gl_Position = vec4(2.0*P-1.0,0,1);
	vec3 V = (vMatrix*gl_Position).xyz;

	float dist = 1.0 - min(1.0,dot(V,V)/ adapt);
	float curv = min(1.0,max(texture(TUc,P).r,texture(TUc,Pa).r)*10);

//	d = min(dist,curv);
	d = 0.0001*dist + curv;

}
)";


static const std::string quad_ctrl = R"(
#version 430
layout (vertices = 4) out;
layout(location=5) uniform int tess;
in float d[];
void main()
{	

	float tmax = float(tess-3);

	float te0 = 3.0 + tmax * (d[0] + d[1])/2.0;
	float te1 = 3.0 + tmax * (d[1] + d[2])/2.0;
	float te2 = 3.0 + tmax * (d[2] + d[3])/2.0;
	float te3 = 3.0 + tmax * (d[3] + d[0])/2.0;

	gl_TessLevelOuter[0] = int(te3);
	gl_TessLevelOuter[1] = int(te0);
	gl_TessLevelOuter[2] = int(te1);
	gl_TessLevelOuter[3] = int(te2);

	gl_TessLevelInner[0] = int((te1+te3)/2.0);
	gl_TessLevelInner[1] = int((te0+te2)/2.0);

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)";

static const std::string quad_eval = R"(
#version 430

layout (quads, equal_spacing, ccw) in;
layout(location=0) uniform mat4 vMatrix;
layout(location=1) uniform mat4 pMatrix;
layout(location=2) uniform mat3 nMatrix;
layout(binding=0) uniform sampler2D TUh;
layout(binding=1) uniform sampler2D TUn;
out vec3 P;
out vec3 N;
out vec2 TC;
void main()
{
	vec2 Qb = mix(gl_in[0].gl_Position.xy,gl_in[1].gl_Position.xy, gl_TessCoord.x);
	vec2 Qt = mix(gl_in[3].gl_Position.xy,gl_in[2].gl_Position.xy, gl_TessCoord.x);
	vec2 Q = mix(Qb,Qt, gl_TessCoord.y);

	TC = 0.5*Q.xy+0.5;
	float h = textureLod(TUh,TC,0).r;

	vec4 P4 = vMatrix * vec4(Q,h,1);
	gl_Position = pMatrix * P4;
	P = P4.xyz;
	N = nMatrix * texture(TUn,TC).xyz;

}
)";

static const std::string tess_frag = R"(
#version 430
out vec3 frag_out;
in vec3 P;
in vec3 N;
in vec2 TC;
layout(binding=3) uniform sampler2D TUim;

void main()
{
	vec3 No = normalize(N);//*0.000001+ normalize(cross(dFdx(P),dFdy(P)));
	vec3 Lu = normalize(- P);
	float lamb = 0.2+0.8*max(0.0,dot(No,Lu));
	frag_out = texture(TUim,TC).rgb*lamb;
}
)";

class Viewer : public GLViewer
{
	ShaderProgram::UP prg_fs;
	ShaderProgram::UP prg_disk;
	ShaderProgram::UP prg_compute;
	ShaderProgram::UP prg_compute_sub;
	ShaderProgram::UP prg_compute_curv;

	Texture2D::SP tex_h;
	Texture2D::SP tex_n;
	Texture2D::SP tex_sub_h;
	Texture2D::SP tex_curv;
	Texture2D::SP tex_im;
	Texture2D::SP tex_no_im;

	FBO::SP fbo;
	VBO::SP vbo;
	VAO::UP vao;

	ShaderProgram::UP prg_quad;
	GLVVec2 P;
	GLVVec2 V;

	float h_max;
	float width_wave;
	float speed;
	int tess;
	bool fill_lines;
	int nbp;
	float adapt;
	int sub_;

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
	h_max(0.15f),
	width_wave(0.25f),
	speed(0.003f),
	tess(32),
	fill_lines(false),
	nbp(32),
	adapt(5.0),
	sub_(8)
{}


void Viewer::init_ogl()
{

	prg_fs = ShaderProgram::create({{GL_VERTEX_SHADER,fs_vert},
										 {GL_FRAGMENT_SHADER,fs_frag}},
										 "fs");

	prg_disk = ShaderProgram::create({{GL_VERTEX_SHADER,disk_vert},
										 {GL_FRAGMENT_SHADER,disk_frag}},
										 "disk");

	prg_quad = ShaderProgram::create({{GL_VERTEX_SHADER,tess_vert},
										 {GL_TESS_CONTROL_SHADER,quad_ctrl},
										 {GL_TESS_EVALUATION_SHADER,quad_eval},
										 {GL_FRAGMENT_SHADER,tess_frag}},
										 "quad_tess");

	prg_compute = ShaderProgram::create({{GL_COMPUTE_SHADER,compute_normal}}, "compute_normal");
	prg_compute_sub = ShaderProgram::create({{GL_COMPUTE_SHADER,compute_sub}}, "compute_sub");
	prg_compute_curv = ShaderProgram::create({{GL_COMPUTE_SHADER,compute_curv}}, "compute_normal");

	const int SZT = 2048;

	tex_h = Texture2D::create();
	tex_h->alloc(SZT,SZT,GL_R32F); // pour le blending
	fbo = FBO::create({tex_h});

	tex_n = Texture2D::create();
	tex_n->alloc(tex_h->width(), tex_h->height(),GL_RGBA32F);

	tex_sub_h = Texture2D::create();
	tex_sub_h->alloc(tex_n->width()/sub_,tex_n->height()/sub_,GL_R32F);

	tex_curv = Texture2D::create();
	tex_curv->alloc(tex_sub_h->width(), tex_sub_h->height(),GL_R32F);

	tex_im = Texture2D::create();
	tex_im->load(DATA_PATH+"/tiger.jpg",true);

//	tex_no_im = Texture2D::create();
//	tex_no_im->

	P.resize(100);
    V.resize(P.size());

	for( std::size_t i=0; i<P.size();++i)
	{
		auto v = (GLVec2{-1.0f+2.0f*float(rand())/RAND_MAX, -1.0f+2.0f*float(rand())/RAND_MAX}).normalized();
		P[i] = GLVec2{-1.0f+2.0f*float(rand())/RAND_MAX, -1.0f+2.0f*float(rand())/RAND_MAX};
		V[i] = v;
	}

	vbo = VBO::create(P);
	vao = VAO::create({{0,vbo,1}});

	set_scene_center(GLVec3(0,0,0));
	set_scene_radius(1.0f);

}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& view = this->get_view_matrix();
//	GLMat4 ipv = (proj*view).inverse();
//	GLVec4 cam4 = ipv*GLVec4(0,0,0,1);
//	GLVec3 cam3 = cam4.block<3,1>(0,0)/cam4.w();
//	std::cout << cam3.transpose()<<std::endl;

//	std::cout << "============================="<<this->camera().focal_dist()<<  std::endl;
//	GLVec3 p = Transfo::apply(view,GLVec3(-1.5f,-1.5f,0));
//	std::cout << -(p.z()+camera().z_near())/(camera().z_far()-camera().z_near()) <<"   -  ";
//	p=Transfo::apply(view,GLVec3(1.5f,1.5f,0));
//	std::cout << -(p.z()+camera().z_near())/(camera().z_far()-camera().z_near()) <<std::endl;


	for( std::size_t i=0; i<P.size();++i)
	{
		P[i] += speed*V[i];
		if (std::abs(P[i].x())>1.0f)
			V[i].x() *=  -1.0f;
		if (std::abs(P[i].y())>1.0f)
			V[i].y() *=  -1.0f;
	}
	vbo->update(P);

	FBO::push();
	fbo->bind();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	prg_disk->bind();
	set_uniform_value("width", width_wave);
	vao->bind();
	glDrawArraysInstanced(GL_TRIANGLE_STRIP,0,4,vbo->length());
	VAO::unbind();
	ShaderProgram::unbind();
	FBO::pop();
	glDisable(GL_BLEND);

	prg_compute->bind();
	set_uniform_value(0, h_max);
	tex_h->bind_compute_in(0);
	tex_n->bind_compute_out(1);
	glDispatchCompute(GLuint(tex_n->width()/32), GLuint(tex_n->height()/32), 1);
//	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	prg_compute_sub->bind();
	set_uniform_value(0,sub_);
	tex_h->bind_compute_in(0);
	tex_sub_h->bind_compute_out(1);
	glDispatchCompute(GLuint(tex_sub_h->width()/32), GLuint(tex_sub_h->height()/32), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	prg_compute_curv->bind();
	tex_sub_h->bind_compute_in(0);
	tex_curv->bind_compute_out(1);
	glDispatchCompute(GLuint(tex_curv->width()/32), GLuint(tex_curv->height()/32), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	Texture2D::unbind_compute_out(1);
	Texture2D::unbind_compute_in(0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//	glDisable(GL_DEPTH_TEST);
//	prg_fs->bind();
//	VAO::none()->bind();
//	glViewport(0,0,1024,1024);
//	set_uniform_value("TU",tex_sub_h->bind(0));
//	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
//	glViewport(1224,0,1024,1024);
//	set_uniform_value("TU",tex_curv->bind(0));
//	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

//	glViewport(2448,0,1024,1024);
//	set_uniform_value("TU",tex_h->bind(0));
//	glDrawArrays(GL_TRIANGLE_STRIP,0,4);


	glEnable(GL_DEPTH_TEST);

	if (fill_lines)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	VAO::none()->bind();

	prg_quad->bind();
	GLMat4 mv = view*Transfo::scale({1.0f,1.0f,h_max});
	set_uniform_value(0, mv);
	set_uniform_value(1, proj);
	set_uniform_value(2, Transfo::inverse_transpose(mv));

	set_uniform_value(3, nbp);
	set_uniform_value(4, h_max);
	set_uniform_value(5, tess);
	set_uniform_value(6, adapt);

	tex_h->bind(0);
	tex_n->bind(1);
	tex_curv->bind(2);
	tex_im->bind(3);

	glPatchParameteri( GL_PATCH_VERTICES, 4 );
	glDrawArrays( GL_PATCHES, 0, 4*nbp*nbp);
	ShaderProgram::unbind();
	Texture2D::unbind();

}

void Viewer::interface_ogl()
{
	ImGui::GetIO().FontGlobalScale = 2.0f;
	ImGui::Begin("EasyCppOGL Terrain",nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::SetWindowSize({0,0});
	ImGui::Text("FPS: %2.2lf", fps_);
	ImGui::Text("MEM:  %2.2lf %%", 100.0*mem_);
	ImGui::SliderFloat("width",&width_wave,0.02,0.5);
	ImGui::SliderFloat("height",&h_max,0.0,0.25);
	ImGui::SliderFloat("speed",&speed,0.0,0.01);
	ImGui::SliderInt("patches",&nbp,2,100);
	ImGui::SliderInt("tess",&tess,4,64);
	ImGui::SliderFloat("adapt dist",&adapt,1.0,100.0);
	ImGui::Checkbox("edges",&fill_lines);
	ImGui::End();
}



