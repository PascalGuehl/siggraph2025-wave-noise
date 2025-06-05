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
#include <texture2darray.h>
#include <iostream>

namespace EZCOGL
{


// TODO

Texture2DArray::Texture2DArray(const std::vector<GLint>& params):
	Texture(),
	height_(0),
	nb_(0)
{
	glGenTextures(1,&id_);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id_);
	simple_params(params);
}


Texture2DArray::~Texture2DArray()
{
	glDeleteTextures(1,&id_);
}

void Texture2DArray::simple_params(const std::vector<GLint>& params)
{
	if (params.empty())
	{
		simple_params({GL_LINEAR,GL_CLAMP_TO_EDGE});
	}
	for(auto p: params)
	{
		if (p == GL_NEAREST || p == GL_LINEAR)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, p);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, p);
		}
        if (p == GL_NEAREST_MIPMAP_NEAREST || p == GL_NEAREST_MIPMAP_LINEAR)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, p);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        if (p == GL_LINEAR_MIPMAP_NEAREST || p == GL_LINEAR_MIPMAP_LINEAR)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, p);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
		if (p == GL_CLAMP_TO_EDGE || p == GL_CLAMP_TO_BORDER || p == GL_REPEAT || p == GL_MIRRORED_REPEAT)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, p);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, p);
		}
	}
}

void Texture2DArray::alloc(GLsizei n, GLsizei w, GLsizei h, GLint internal, const GLubyte* ptr)
{
    internal_ = internal;
    auto tf = Texture::texture_formats[internal];
	external_ = tf.first;
	width_ = w;
	height_ = h;
	data_type_ = tf.second;
	bind();
	glTexImage3D(GL_TEXTURE_2D, 0, internal, w, h, n, 0, external_, data_type_, (w * h * n > 0)?ptr:nullptr);
	unbind();
}


}
