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

#ifndef EASY_CPP_OGL_UBO_H_
#define EASY_CPP_OGL_UBO_H_

#include <GL/gl3w.h>
#include <vector>
#include <gl_eigen.h>
#include <memory>

namespace EZCOGL
{

class  UBO
{
       static GLuint binding_points_;
protected:
	GLuint id_;
    GLuint binding_point_index_;
    GLuint size_;

    template<typename ST>
    UBO(const ST& data, GLuint bp):
        size_(sizeof(ST))
    {
        glGenBuffers(1, &id_);
        glBindBuffer(GL_UNIFORM_BUFFER, id_);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(ST), nullptr, GL_DYNAMIC_COPY);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        binding_point_index_ = bp;
        glBindBufferBase(GL_UNIFORM_BUFFER, binding_point_index_, id_);
	}

public:
    using UP = std::unique_ptr<UBO>;

    UBO(const UBO&) = delete;


    template<typename ST>
    static UBO::UP create(const ST& data , GLuint bp)
	{
        return std::unique_ptr<UBO>(new UBO(data, bp));
	}

    inline ~UBO()
	{
		glDeleteBuffers(1,&id_);
		id_ = 0;
	}

	inline void bind()
	{
        glBindBuffer(GL_UNIFORM_BUFFER, id_);
	}

	inline static void unbind()
	{
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}


	inline GLuint id() const
	{
		return id_;
	}

    template<typename ST>
    void update(const ST *ptr, GLsizei offset = 0, GLsizei size = 0)
    {
        glBufferSubData(GL_UNIFORM_BUFFER, offset, size==0?size_:size,ptr);
    }

};

} // namespace
#endif // EASY_CPP_OGL_UBO_H_
