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

#ifndef EASY_CPP_OGL_TEXTURE1D_ARRAY_H_
#define EASY_CPP_OGL_TEXTURE1D_ARRAY_H_

#include <texture.h>

namespace EZCOGL
{

class Texture1DArray:public Texture
{
protected:
    GLsizei nb_;

    Texture1DArray(const std::vector<GLint>& params = {});

public:
    using UP = std::unique_ptr<Texture1DArray>;

    ~Texture1DArray();

    static Texture1DArray::UP create(const std::vector<GLint>& params = {})
    {
        return std::unique_ptr<Texture1DArray>(new Texture1DArray(params));
    }

    Texture1DArray(const Texture1DArray&) = delete ;

    inline GLuint id()
    {
        return id_;
    }

    void simple_params(const std::vector<GLint>& params);

    void alloc(GLsizei n, GLsizei w, GLint internal, const GLubyte* ptr=nullptr);

    template <typename T>
    void update(GLint n, GLint x, GLint w, const T* data);

    inline void bind() { glBindTexture(GL_TEXTURE_1D_ARRAY, this->id_); }

    inline GLint bind(GLint unit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_1D_ARRAY, id_);
        return unit;
    }

    inline static void unbind()
    {
        glBindTexture(GL_TEXTURE_1D_ARRAY,0);
    }


};

template<typename T>
void Texture1DArray::update(GLint n, GLint x, GLint w, const T *data)
{
    bind();
    glTexSubImage2D(GL_TEXTURE_1D_ARRAY, 0, x,n,w,1, external_, data_type_, reinterpret_cast<const void*>(data));
    unbind();
}


}
#endif
