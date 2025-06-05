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

#ifndef EASY_CPP_OGL_TEXTURE_CUBE_H_
#define EASY_CPP_OGL_TEXTURE_CUBE_H_

#include <texture.h>
#include <array>

namespace EZCOGL
{


class TextureCubeMap:public Texture
{
protected:
    GLsizei height_;

    TextureCubeMap(const std::vector<GLint>& params = {});

public:
    using SP = std::shared_ptr<TextureCubeMap>;

    ~TextureCubeMap();

    static TextureCubeMap::SP create(const std::vector<GLint>& params = {})
    {
        return std::shared_ptr<TextureCubeMap>(new TextureCubeMap(params));
    }

    TextureCubeMap(const TextureCubeMap&) = delete ;

    inline GLuint id()
    {
        return id_;
    }

    void simple_params(const std::vector<GLint>& params);

    void alloc(GLsizei w, GLint internal, const std::vector<const GLubyte *> ptrs);

    void load(const std::array<std::string,6>& filenames);

    template <typename T>
    void update(GLint im, GLint x, GLint y, GLint w, GLint h, const T* data);


    inline void bind()
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    }

    inline GLint bind(GLint unit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
        return unit;
    }

    inline static void unbind()
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP,0);
    }

};

template <typename T>
void TextureCubeMap::update(GLint im, GLint x, GLint y, GLint w, GLint h, const T* data)
{
    bind();
    glTexSubImage3D(GL_TEXTURE_3D, 0, x,y,im,w,h,1, external_, data_type_, data);
    unbind();
}


}
#endif
