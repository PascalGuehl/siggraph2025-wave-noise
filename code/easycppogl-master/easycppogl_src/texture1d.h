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

#ifndef EASY_CPP_OGL_TEXTURE1D_H_
#define EASY_CPP_OGL_TEXTURE1D_H_


#include <memory>
#include <texture.h>

namespace EZCOGL
{

class Texture1D: public Texture
{
protected:

    Texture1D(const std::vector<GLint>& params = {});

public:
    using SP = std::shared_ptr<Texture1D>;

    ~Texture1D();

    static Texture1D::SP create(const std::vector<GLint>& params = {})
    {
        return std::shared_ptr<Texture1D>(new Texture1D(params));
    }

    Texture1D(const Texture1D&) = delete ;

    inline GLuint id()
    {
        return this->id_;
    }

    void simple_params(const std::vector<GLint>& params);

    void alloc(GLsizei w, GLint internal, const GLubyte* ptr=nullptr);

    template <typename T>
    void update(GLint x, GLint w, const T* data);

    inline void bind()
    {
        glBindTexture(GL_TEXTURE_1D, id_);
    }

    inline GLint bind(GLint unit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_1D, id_);
        return unit;
    }

    inline static void unbind()
    {
        glBindTexture(GL_TEXTURE_1D,0);
    }


};

template <typename T>
void Texture1D::update(GLint x, GLint w, const T* data)
{
    bind();
    glTexSubImage1D(GL_TEXTURE_1D, 0, x,w, external_, data_type_, reinterpret_cast<const void*>(data));
    unbind();
}


}
#endif
