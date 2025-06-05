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

#ifndef EASY_CPP_OGL_TEXTURE2D_ARRAY_H_
#define EASY_CPP_OGL_TEXTURE2D_ARRAY_H_

#include <texture.h>

namespace EZCOGL
{



class Texture2DArray:public Texture
{
protected:
    GLsizei height_;
    GLsizei nb_;

    Texture2DArray(const std::vector<GLint>& params = {});

public:
    using SP = std::shared_ptr<Texture2DArray>;

    ~Texture2DArray();

    static Texture2DArray::SP create(const std::vector<GLint>& params = {})
    {
        return std::shared_ptr<Texture2DArray>(new Texture2DArray(params));
    }

    Texture2DArray(const Texture2DArray&) = delete ;

    void simple_params(const std::vector<GLint>& params);

    void alloc(GLsizei nb, GLsizei w, GLsizei h, GLint internal, const GLubyte* ptr=nullptr);

    template<typename T>
    void update(GLint nt, GLint x, GLint y, GLint w, GLint h, const T *data);

    inline GLsizei height() const { return height_; }

    inline GLsizei nb() const
    {
        return nb_;
    }

    inline void bind()
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY, id_);
    }

    inline GLint bind(GLint unit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D_ARRAY, id_);
        return unit;
    }

    inline static void unbind()
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY,0);
    }

};

template <typename T>
void Texture2DArray::update(GLint nt, GLint x, GLint y, GLint w, GLint h, const T* data)
{
    bind();
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, x,y,nt,w,h,1, external_, data_type_, reinterpret_cast<const void*>(data));
    unbind();
}


}
#endif
