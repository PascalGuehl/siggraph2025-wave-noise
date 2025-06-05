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

#include <texturecubemap.h>
#include <texture2d.h>
#include <memory>

namespace EZCOGL {


TextureCubeMap::TextureCubeMap(const std::vector<GLint> &params):
    height_(0)
{
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    simple_params(params);
}


TextureCubeMap::~TextureCubeMap()
{
    glDeleteTextures(1, &id_);
}

void TextureCubeMap::simple_params(const std::vector<GLint> &params)
{
    if (params.empty()) {
        simple_params({GL_LINEAR, GL_CLAMP_TO_EDGE});
    }
    for (auto p : params) {
        if (p == GL_NEAREST || p == GL_LINEAR) {
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, p);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, p);
        }
        if (p == GL_NEAREST_MIPMAP_NEAREST || p == GL_NEAREST_MIPMAP_LINEAR) {
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, p);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        if (p == GL_LINEAR_MIPMAP_NEAREST || p == GL_LINEAR_MIPMAP_LINEAR) {
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, p);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        if (p == GL_CLAMP_TO_EDGE || p == GL_CLAMP_TO_BORDER || p == GL_REPEAT
            || p == GL_MIRRORED_REPEAT) {
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, p);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, p);
        }
    }
}

void TextureCubeMap::alloc(GLsizei w, GLint internal, const std::vector<const GLubyte *> ptrs)
{
    internal_ = internal;
    auto tf = Texture::texture_formats[internal];
    external_ = tf.first;
    width_ = w;
    data_type_ = tf.second;
    if (w > 0) {
        bind();
        for (int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0,
                         internal_,
                         w,
                         w,
                         0,
                         external_,
                         data_type_,
                         ptrs[i]);
        unbind();
    }
}



void TextureCubeMap::load(const std::array<std::string,6>& filenames)
{
    // store images to avoid destruction of buffer before GL copy to text
    std::vector < std::unique_ptr<GLImage>> imgs;
    for (const auto& fn : filenames)
      imgs.emplace_back(new GLImage(fn, false));
  //       imgs.push_back(std::make_unique<GLImage>(fn,false));

    std::vector<const GLubyte*> data;
    for (const auto& im : imgs)
        data.push_back(im->data());

    int depth = imgs.front()->depth();
    int w = imgs.front()->width();
   
    switch (depth)
    {
    case 1:
        alloc(w,GL_R8,data);
        break;
    case 3:
        alloc(w,GL_RGB8, data);
        break;
    case 4:
        alloc(w,GL_RGBA8, data);
        break;
	}
    // end of scope for imgs all unique_ptrs are destroyed, image are freed
}


}
