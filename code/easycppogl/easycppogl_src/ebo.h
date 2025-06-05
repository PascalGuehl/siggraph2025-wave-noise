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

#ifndef EASY_CPP_OGL_EBO_H_
#define EASY_CPP_OGL_EBO_H_

#include <GL/gl3w.h>
#include<vector>
#include<memory>

namespace EZCOGL
{

class EBO;

class EBO
{
protected:
	GLuint id_;
	GLuint nb_;

	inline EBO():
		nb_(0)
	{
		glGenBuffers(1, &id_);
	}

public:

	using SP = std::shared_ptr<EBO>;

	void init(const std::vector<GLuint>& buffer)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(buffer.size()*sizeof(GLuint)), buffer.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		nb_ = GLuint(buffer.size());
	}


	inline static EBO::SP create(const std::vector<GLuint>& buffer)
	{
		auto ptr = new EBO();
		ptr->init(buffer);
		return std::shared_ptr<EBO>(ptr);
	}

	inline ~EBO()
	{
		glDeleteBuffers(1,&id_);
	}

	inline void bind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
	}

	inline static void unbind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	inline void allocate(GLuint nb_ind)
	{
		if (nb_ind != nb_) // only allocate when > ?
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(nb_ind*sizeof(GLuint)), nullptr, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			nb_ = nb_ind;
		}
	}

	/**
	 * @brief update
	 * @param buffer source
	 */
	inline void update(const std::vector<GLuint>& buffer)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0u, GLsizeiptr(buffer.size()*sizeof(GLuint)), buffer.data());
	}

	/**
	 * @brief update_sub
	 * @param offset offset in vec
	 * @param buffer source
	 * @param nb vec if not all buffer
	 */
	inline void update_sub(GLuint offset, const std::vector<GLuint>& buffer, GLuint nb = 0)
	{
		GLsizeiptr nb_oct = nb ? GLsizeiptr(nb * sizeof(GLuint)) : GLsizeiptr(buffer.size() * sizeof(GLuint));
		glBindBuffer(GL_ARRAY_BUFFER, id_);
		glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(GLuint), nb_oct, buffer.data());
	}

	inline GLint length() const
	{
		return GLint(nb_);
	}

	inline GLuint id() const
	{
		return id_;
	}
};

} // namespace

#endif // EASY_CPP_OGL_EBO_H_
