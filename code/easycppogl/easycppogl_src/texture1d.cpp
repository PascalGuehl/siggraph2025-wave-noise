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

#include <texture1d.h>
#include <iostream>

namespace EZCOGL
{

std::map<GLint,std::pair<GLenum,GLenum>> Texture::texture_formats =
{
 {GL_R8,{GL_RED,GL_UNSIGNED_BYTE}},
 {GL_RG8,{GL_RG,GL_UNSIGNED_BYTE}},
 {GL_RGB8,{GL_RGB,GL_UNSIGNED_BYTE}},
 {GL_RGBA8,{GL_RGBA,GL_UNSIGNED_BYTE}},
 {GL_R16,{GL_RED,GL_UNSIGNED_SHORT}},
 {GL_RG16,{GL_RG,GL_UNSIGNED_SHORT}},
 {GL_RGB16,{GL_RGB,GL_UNSIGNED_SHORT}},
 {GL_RGBA16,{GL_RGBA,GL_UNSIGNED_SHORT}},
 {GL_R16F,{GL_RED,GL_HALF_FLOAT}},
 {GL_RG16F,{GL_RG,GL_HALF_FLOAT}},
 {GL_RGB16F,{GL_RGB,GL_HALF_FLOAT}},
 {GL_RGBA16F,{GL_RGBA,GL_HALF_FLOAT}},
 {GL_R32F,{GL_RED,GL_FLOAT}},
 {GL_RG32F,{GL_RG,GL_FLOAT}},
 {GL_RGB32F,{GL_RGB,GL_FLOAT}},
 {GL_RGBA32F,{GL_RGBA,GL_FLOAT}},
 {GL_R16I,{GL_RED_INTEGER,GL_SHORT}},
 {GL_RG16I,{GL_RG_INTEGER,GL_SHORT}},
 {GL_RGB16I,{GL_RGB_INTEGER,GL_SHORT}},
 {GL_RGBA16I,{GL_RGBA_INTEGER,GL_SHORT}},
 {GL_R32I,{GL_RED_INTEGER,GL_INT}},
 {GL_RG32I,{GL_RG_INTEGER,GL_INT}},
 {GL_RGB32I,{GL_RGB_INTEGER,GL_INT}},
 {GL_RGBA32I,{GL_RGBA_INTEGER,GL_INT}},
 {GL_R16UI,{GL_RED_INTEGER,GL_UNSIGNED_SHORT}},
 {GL_RG16UI,{GL_RG_INTEGER,GL_UNSIGNED_SHORT}},
 {GL_RGB16UI,{GL_RGB_INTEGER,GL_UNSIGNED_SHORT}},
 {GL_RGBA16UI,{GL_RGBA_INTEGER,GL_UNSIGNED_SHORT}},
 {GL_R32UI,{GL_RED_INTEGER,GL_UNSIGNED_INT}},
 {GL_RG32UI,{GL_RG_INTEGER,GL_UNSIGNED_INT}},
 {GL_RGB32UI,{GL_RGB_INTEGER,GL_UNSIGNED_INT}},
 {GL_RGBA32UI,{GL_RGBA_INTEGER,GL_UNSIGNED_INT}},
 {GL_RGB10_A2,{GL_RGBA,GL_UNSIGNED_INT_2_10_10_10_REV}},
 {GL_DEPTH_COMPONENT,  {GL_DEPTH_COMPONENT,GL_UNSIGNED_INT}},
 {GL_DEPTH_COMPONENT16,{GL_DEPTH_COMPONENT,GL_UNSIGNED_SHORT}},
 {GL_DEPTH_COMPONENT24,{GL_DEPTH_COMPONENT,GL_UNSIGNED_INT}},
 {GL_DEPTH_COMPONENT32F,{GL_DEPTH_COMPONENT,GL_FLOAT}}
};


Texture1D::Texture1D(const std::vector<GLint>& params)
{
	glGenTextures(1,&id_);
    glBindTexture(GL_TEXTURE_1D, id_);
	simple_params(params);
}


Texture1D::~Texture1D()
{
	glDeleteTextures(1,&id_);
}

void Texture1D::simple_params(const std::vector<GLint>& params)
{
	if (params.empty())
	{
		simple_params({GL_LINEAR,GL_CLAMP_TO_EDGE});
	}
	for(auto p: params)
	{
		if (p == GL_NEAREST || p == GL_LINEAR)
		{
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, p);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, p);
		}
        if (p == GL_NEAREST_MIPMAP_NEAREST || p == GL_NEAREST_MIPMAP_LINEAR)
        {
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, p);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        if (p == GL_LINEAR_MIPMAP_NEAREST || p == GL_LINEAR_MIPMAP_LINEAR)
        {
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, p);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
		if (p == GL_CLAMP_TO_EDGE || p == GL_CLAMP_TO_BORDER || p == GL_REPEAT || p == GL_MIRRORED_REPEAT)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, p);
		}
	}
}

void Texture1D::alloc(GLsizei w, GLint internal, const GLubyte* ptr)
{
	internal_ = internal;
    auto tf = Texture::texture_formats[internal];
	external_ = tf.first;
	width_ = w;
	data_type_ = tf.second;
	depth_ = ((internal == GL_DEPTH_COMPONENT32F) || (internal == GL_DEPTH_COMPONENT24));
	bind();
	glTexImage1D(GL_TEXTURE_1D, 0, internal, w, 0, external_, data_type_, (w > 0)?ptr:nullptr);
	unbind();
}



}
