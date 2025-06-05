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
#include "texture2d.h"
#include "vao.h"
#include "vbo.h"
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

static const std::string fs_vert = R"(
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

class Viewer : public GLViewer
{
	ShaderProgram::UP prg1_;
	MeshRenderer::UP renderer_;
	FBO_Depth::SP fbo;
	ShaderProgram::UP prg_fs;

	std::shared_ptr<pfd::open_file> file_dialog;
	std::vector<std::string> supported_files;

public:
	Viewer();
	void init_ogl() override;
	void draw_ogl() override;
	void resize_ogl(int32_t w, int32_t h) override;
};

int main(int, char**)
{
	Viewer v;
	return v.launch3d();
}

Viewer::Viewer() : file_dialog(nullptr), supported_files({"Mesh Files", "*.off *.obj *.ply", "All Files", "*"})
{
}

void Viewer::resize_ogl(int32_t w, int32_t h)
{
	fbo->resize(w, h);
}

void Viewer::init_ogl()
{
	prg1_ = ShaderProgram::create({{GL_VERTEX_SHADER, p_vert}, {GL_FRAGMENT_SHADER, p_frag}}, "pos");
	prg_fs = ShaderProgram::create({{GL_VERTEX_SHADER, fs_vert}, {GL_FRAGMENT_SHADER, fs_frag}}, "FullScreen");

	auto me = Mesh::Wave(60);
	renderer_ = me->renderer(1, -1, -1, -1, -1);

	set_scene_center(me->BB()->center());
	set_scene_radius(me->BB()->radius());

	auto t = Texture2D::create({GL_NEAREST, GL_CLAMP_TO_EDGE});
	t->init(GL_RGB8);
	fbo = FBO_Depth::create({t});
}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();

	FBO::push();
	fbo->bind();
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

	prg1_->bind();
	set_uniform_value(1, proj * mv);
	set_uniform_value(2, GLVec3{1, 1, 0});
	renderer_->draw(GL_LINES);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, 1.1);

	set_uniform_value(2, GLVec3{0, 0, 0.8f});
	renderer_->draw(GL_TRIANGLES);

	glDisable(GL_POLYGON_OFFSET_FILL);

	FBO::pop();
	glDisable(GL_DEPTH_TEST);
	prg_fs->bind();
	set_uniform_value(2, 1.0f);
	set_uniform_value(1, fbo->texture(0)->bind(0));
	VAO::none()->bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

