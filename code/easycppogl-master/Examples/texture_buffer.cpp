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
#include "texturebuffer.h"
#include "texture2d.h"

#include "gl_viewer.h"
#include <GLFW/glfw3.h>

#define macro_str(s) #s
#define macro_xstr(s) macro_str(s)
#define DATA_PATH std::string(macro_xstr(DEF_DATA_PATH))

using namespace EZCOGL;


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
uniform samplerBuffer TU;
uniform usamplerBuffer TUi;
void main()
{
	int i = int(floor(tc.y*4.0));
	uint j = texelFetch(TUi,i).r;
	frag_out = texelFetch(TU,int(j)).rgb;
}
)";


class Viewer: public GLViewer
{
	ShaderProgram::UP prg_fs;
	SP_TextureBuffer tbuffer_;
	SP_TextureUIBuffer tibuffer_;
	VBO::SP vbo;
	EBO::SP ebo;

public:
	Viewer();
	void init_ogl() override;
	void draw_ogl() override;
	void interface_ogl() override;
	void key_press_ogl(int32_t key_code) override;

};

int main(int, char**)
{
	Viewer v;
	return v.launch3d();
}

Viewer::Viewer()
{}

void Viewer::key_press_ogl(int32_t key_code)
{
	switch(key_code)
	{
		case GLFW_KEY_SPACE:
            tbuffer_->vbo().update(GLVVec3{{1,1,0},{0,1,1}},0,2);
			tibuffer_->ebo().update({0,2,1,3});
			break;
	}

}

void Viewer::init_ogl()
{
	prg_fs = ShaderProgram::create({{GL_VERTEX_SHADER,fs_vert},{GL_FRAGMENT_SHADER,fs_frag}}, "FullScreen");

	vbo = VBO::create(GLVVec3{{1,1,1}, {1,0,0}, {0,1,0}, {0,0,1}});
	tbuffer_ = TextureBuffer::create(vbo);

	ebo = EBO::create({0,2,1,3});
	tibuffer_ = TextureUIBuffer::create(ebo);

}

void Viewer::draw_ogl()
{

	glDisable(GL_DEPTH_TEST);
	prg_fs->bind();
	set_uniform_value("TU", tbuffer_->bind(0));
	set_uniform_value("TUi", tibuffer_->bind(1));
	VAO::none()->bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

void Viewer::interface_ogl()
{}
