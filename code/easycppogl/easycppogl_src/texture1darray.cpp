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
#include <texture.h>
#include <texture1darray.h>
#include <iostream>

namespace EZCOGL
{

Texture1DArray::Texture1DArray(const std::vector<GLint>& params):
	Texture(),
	nb_(0)
{
	glGenTextures(1,&id_);
    glBindTexture(GL_TEXTURE_1D, id_);
	simple_params(params);
}


Texture1DArray::~Texture1DArray()
{
	glDeleteTextures(1,&id_);
}

void Texture1DArray::simple_params(const std::vector<GLint>& params)
{
	if (params.empty())
	{
		simple_params({GL_LINEAR,GL_CLAMP_TO_EDGE});
	}
	for(auto p: params)
	{
		if (p == GL_NEAREST || p == GL_LINEAR)
		{
            glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MIN_FILTER, p);
            glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MAG_FILTER, p);
		}
        if (p == GL_NEAREST_MIPMAP_NEAREST || p == GL_NEAREST_MIPMAP_LINEAR)
        {
            glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MIN_FILTER, p);
            glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        if (p == GL_LINEAR_MIPMAP_NEAREST || p == GL_LINEAR_MIPMAP_LINEAR)
        {
            glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MIN_FILTER, p);
            glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
		if (p == GL_CLAMP_TO_EDGE || p == GL_CLAMP_TO_BORDER || p == GL_REPEAT || p == GL_MIRRORED_REPEAT)
		{
            glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_WRAP_S, p);
		}
	}
}

void Texture1DArray::alloc(GLsizei n, GLsizei w, GLint internal, const GLubyte* ptr)
{
	internal_ = internal;
    auto tf = Texture::texture_formats[internal];
	external_ = tf.first;
	width_ = w;
	nb_ = n;
	data_type_ = tf.second;
	depth_ = ((internal == GL_DEPTH_COMPONENT32F) || (internal == GL_DEPTH_COMPONENT24));
	bind();
	glTexImage2D(GL_TEXTURE_1D_ARRAY, 0, internal, w, nb_, 0, external_, data_type_, (w * n > 0)?ptr:nullptr);
	unbind();
}


}
