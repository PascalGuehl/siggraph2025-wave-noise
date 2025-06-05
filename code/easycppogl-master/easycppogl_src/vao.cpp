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

#include <vao.h>

namespace EZCOGL
{

VAO::UP VAO::none_ = nullptr;

const std::unique_ptr<VAO>& VAO::none()
{
	if (none_ == nullptr)
		none_ = std::unique_ptr<VAO>(new VAO(std::vector<std::tuple<GLint,VBO::SP>>()));
	return none_;
}

void VAO::bind_none()
{
	VAO::none()->bind();
}



VAO::~VAO()
{
	glDeleteVertexArrays(1, &id_);
}



VAO::VAO(const std::vector<std::tuple<GLint, std::shared_ptr<VBO>>>& att_vbo)
{
	glGenVertexArrays(1, &id_);
	if (att_vbo.empty())
	{
		nb_ = 0u;
		return;
	}
	add(att_vbo);
}

void VAO::add(const std::vector<std::tuple<GLint, std::shared_ptr<VBO>>>& att_vbo)
{
	glBindVertexArray(id_);
	for (const auto& p: att_vbo)
	{
		store_vbo(p);
		glBindBuffer(GL_ARRAY_BUFFER, std::get<1>(p)->id());
		GLuint vid = GLuint(std::get<0>(p));
		glEnableVertexAttribArray(vid);
		glVertexAttribPointer(vid, GLint(std::get<1>(p)->vector_dimension()), GL_FLOAT, GL_FALSE, 0, nullptr);
		nb_ = std::min(GLuint(std::get<1>(p)->length()),nb_);
	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VAO::VAO(const std::vector<std::tuple<GLint, VBO::SP, GLint>>& att_vbo)
{
	glGenVertexArrays(1, &id_);
	if (att_vbo.empty())
	{
		nb_ = 0u;
		return;
	}
	add(att_vbo);
}

void VAO::add(const std::vector<std::tuple<GLint, VBO::SP, GLint>>& att_vbo)
{
	glBindVertexArray(id_);
	for (const auto& p: att_vbo)
	{
		store_vbo(p);
		const VBO::SP& vbo = std::get<1>(p);
		glBindBuffer(GL_ARRAY_BUFFER, vbo->id());
		GLuint vid = GLuint(std::get<0>(p));
		glEnableVertexAttribArray(vid);
		glVertexAttribPointer(vid, GLint(vbo->vector_dimension()), GL_FLOAT, GL_FALSE, 0, nullptr);
		glVertexAttribDivisor(vid, std::get<2>(p));
		if (std::get<2>(p) > 0)
			nb_ = std::min(GLuint(vbo->length()),nb_);
	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}



VAO::VAO(const std::vector<std::tuple<GLint, std::shared_ptr<VBO>, GLint, GLint, GLint>>& att_vbo)
{
	glGenVertexArrays(1, &id_);
	if (att_vbo.empty())
	{
		nb_ = 0u;
		return;
	}
	add(att_vbo);
}

void VAO::add(const std::vector<std::tuple<GLint, std::shared_ptr<VBO>, GLint, GLint, GLint>>& att_vbo)
{
	glBindVertexArray(id_);
	for (const auto& p: att_vbo)
	{
		store_vbo(p);
		const VBO::SP& vbo = std::get<1>(p);
		glBindBuffer(GL_ARRAY_BUFFER, vbo->id());
		GLuint vid = GLuint(std::get<0>(p));
		glEnableVertexAttribArray(vid);
		glVertexAttribPointer(vid, GLint(vbo->vector_dimension()), GL_FLOAT, GL_FALSE, std::get<2>(p)*sizeof(GLfloat), reinterpret_cast<void*>(std::get<3>(p)*sizeof(GLfloat)));
		glVertexAttribDivisor(vid, std::get<4>(p));
		if (std::get<4>(p) > 0)
			nb_ = std::min(GLuint(vbo->length()),nb_);

	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}



void VAO::remove(GLint att_loc, VBO::SP vbo)
{
	//TODO
}



}
