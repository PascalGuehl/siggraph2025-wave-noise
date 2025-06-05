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

#include "gl_viewer.h"
#include "shader_program.h"
#include <iostream>

using namespace EZCOGL;


static const std::string prg1_vert = R"(
#version 430

layout(location=1) in vec3 position_in;
layout(location=2) in float intensity_in;
layout(location=1) uniform mat4 pmvMatrix;
out float intensity;
void main()
{
	gl_Position = pmvMatrix * vec4(position_in,1);
	intensity = intensity_in;
}
)";

static const std::string prg1_frag = R"(
#version 430
in float intensity;
layout(location=2) uniform vec3 color1;
layout(location=3) uniform vec3 color2;
out vec3 frag_out;
void main()
{
	frag_out = mix(color1,color2,intensity);
}
)";


static const std::string prg2_vert = R"(
#version 430

layout(location=1) in vec3 position_in;
layout(location=2) in float intensity_in;
flat out float intensity;
void main()
{
	gl_Position =vec4(position_in,1);
	intensity = intensity_in;
}
)";

static const std::string prg2_geom = R"(
#version 430

layout(points) in;
layout(line_strip, max_vertices = 2) out;
flat in float intensity[];
flat out float line_intensity;
layout(location=1) uniform mat4 pmvMatrix;
void main()
{
	line_intensity = intensity[0];
	gl_Position = pmvMatrix*(gl_in[0].gl_Position);
	EmitVertex();
	line_intensity = intensity[0];
	vec4 P2 = vec4(gl_in[0].gl_Position.xyz + 0.1*normalize(gl_in[0].gl_Position.xyz),1);
	gl_Position = pmvMatrix*P2 ;
	EmitVertex();
	EndPrimitive();
}
)";


static const std::string prg2_frag = R"(
#version 430
flat in float line_intensity;
layout(location=2) uniform vec3 color1;
layout(location=3) uniform vec3 color2;
out vec3 frag_out;
void main()
{
	frag_out = mix(color1,color2,line_intensity);
}
)";


class Viewer : public GLViewer
{
	ShaderProgram::UP prg_;
	ShaderProgram::UP prg_geom_;
	VAO::UP vao1_;
	int nbp_;

public:
	Viewer();
	void init_ogl() override;
	void draw_ogl() override;
};

int main(int, char**)
{
	Viewer v;
	return v.launch3d();
}

inline Viewer::Viewer()
{
}

void Viewer::init_ogl()
{
	prg_ = ShaderProgram::create({{GL_VERTEX_SHADER, prg1_vert}, {GL_FRAGMENT_SHADER, prg1_frag}}, "prg_vf");

	prg_geom_ = ShaderProgram::create(
		{{GL_VERTEX_SHADER, prg2_vert}, {GL_GEOMETRY_SHADER, prg2_geom} , {GL_FRAGMENT_SHADER, prg2_frag}}, "prg_vgf");

	const int N = 11;
	GLVVec3 P;
	P.reserve(N * N * N);
	std::vector<float> I;
	I.reserve(N * N * N);
	for (int i=0;i<N;++i)
	{
		for (int j = 0; j < N; ++j)
		{
			for (int k = 0; k < N; ++k)
			{
				EZCOGL::GLVec3 po =	GLVec3(float(k) / float(N) * 2 - 1, float(j) / float(N) * 2 - 1, float(i) / float(N) * 2 - 1);
				P.push_back(po);
				//P.emplace_back(float(k) / float(N) * 2 - 1, float(j) / float(N) * 2 - 1, float(i) / float(N) * 2 - 1);
				I.push_back(std::min(1.0f, po.norm()));
			}
		
		}
	}
	VBO::SP vbo_p = VBO::create(P);
	VBO::SP vbo_i = VBO::create(I);
	vao1_ = VAO::create({
		{1, vbo_p},
		{2, vbo_i},
	});
	nbp_ = P.size();

	set_scene_center(GLVec3{0, 0, 0});
	set_scene_radius(3);
}

void Viewer::draw_ogl()
{
	const GLMat4& proj = this->get_projection_matrix();
	const GLMat4& mv = this->get_view_matrix();
	GLMat4 pmv = proj * mv;
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	prg_->bind();
	set_uniform_value(1, pmv);
	set_uniform_value(2, GLVec3{1, 0, 0});
	set_uniform_value(3, GLVec3{0, 1, 1});
	vao1_->bind();
	glPointSize(5);
	glDrawArrays(GL_POINTS, 0, nbp_);

	prg_geom_->bind();
	set_uniform_value(1, pmv);
	set_uniform_value(2, GLVec3{1, 0, 0});
	set_uniform_value(3, GLVec3{0, 1, 1});
	vao1_->bind();
	glDrawArrays(GL_POINTS, 0, nbp_);
}
