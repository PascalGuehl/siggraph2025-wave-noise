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

#ifndef EASY_CPP_OGL_TEXTURE2D_H_
#define EASY_CPP_OGL_TEXTURE2D_H_

#include <GL/gl3w.h>
#include <texture.h>

namespace EZCOGL
{


	class GLImage
    {
        GLubyte* data_;
        GLint width_;
        GLint height_;
        GLint bpp_;

    public:
    inline GLImage() :
            data_(nullptr), width_(0), height_(0), bpp_(0)
        {}
    GLImage(const std::string& filename, bool yinvert = true, int force_channels = 0);
    ~GLImage();
    inline GLint depth() const { return bpp_;}
    inline GLint width() const { return width_;}
    inline GLint height() const { return height_;}
    inline const GLubyte* data() const { return data_;}
};

class Texture2D:public Texture
{
protected:

    GLsizei height_;
    bool depth_;

    Texture2D(const std::vector<GLenum>& params = {});

public:
    using SP = std::shared_ptr<Texture2D>;

    ~Texture2D();

    static Texture2D::SP create(const std::vector<GLenum>& params = {})
    {
        return Texture2D::SP(new Texture2D(params));
    }

    Texture2D(const Texture2D&) = delete ;



    void simple_params(const std::vector<GLenum>& params);

    void alloc(GLsizei w, GLsizei h, GLint internal, const GLubyte* ptr=nullptr);

    void init(GLint internal);

	bool load(const std::string& filename, GLint force_nb_channel = 0);

    void resize(GLsizei w, GLsizei h);

    template <typename T>
    void update(GLint x, GLint y, GLint w, GLint h, const T* data);



    inline GLsizei height() const
    {
        return height_;
    }

    inline void bind()
    {
        glBindTexture(GL_TEXTURE_2D, id());
    }

    inline GLint bind(GLint unit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id());
        return unit;
    }

    inline static void unbind()
    {
        glBindTexture(GL_TEXTURE_2D,0);
    }


};

template <typename T>
void Texture2D::update(GLint x, GLint y,GLint w, GLint h, const T* data)
{
    bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, x,y,w,h, external_, data_type_, reinterpret_cast<const void*>(data));
    unbind();
}


}
#endif
