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
#include <texture2d.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>

namespace EZCOGL
{


GLImage::																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								GLImage(const std::string& filename, bool yinvert , int force_channels)
{
	stbi_set_flip_vertically_on_load(yinvert?1:0);
	data_ = stbi_load(filename.c_str(), &width_, &height_, &bpp_, force_channels);
	if (force_channels !=0)
		bpp_ = force_channels;
	if (data_ == nullptr)
	{
		width_=0;
		height_=0;
		bpp_=0;
		std::cerr << "Error while loading "<< filename << std::endl;
	}
}

GLImage::~GLImage()
{
	stbi_image_free(data_);
}


Texture2D::Texture2D(const std::vector<GLenum>& params):
	Texture(),
	height_(0)
{
	glGenTextures(1,&id_);
	glBindTexture(GL_TEXTURE_2D, id_);
	simple_params(params);
}


Texture2D::~Texture2D()
{
	glDeleteTextures(1,&id_);
}

void Texture2D::simple_params(const std::vector<GLenum>& params)
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

void Texture2D::alloc(GLsizei w, GLsizei h, GLint internal, const GLubyte* ptr)
{
    internal_ = internal;
    auto tf = Texture::texture_formats[internal];
	external_ = tf.first;
	width_ = w;
	height_ = h;
	data_type_ = tf.second;
	depth_ = ((internal == GL_DEPTH_COMPONENT32F) || (internal == GL_DEPTH_COMPONENT24));
	bind();
	glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, external_, data_type_, (w * h > 0) ? ptr: nullptr);
	unbind();
}


void Texture2D::init(GLint internal)
{
	alloc(0, 0, internal, nullptr);
}


bool Texture2D::load(const std::string& filename, GLint force_nb_channel)
{
	GLImage img(filename, true, force_nb_channel);
	std::cout << "TRY TO LOAD: " << filename;

	switch (img.depth())
	{
		case 1:
			alloc(img.width(),img.height(),GL_R8, img.data());
			break;
		case 3:
			alloc(img.width(),img.height(),GL_RGB8, img.data());
			break;
		case 4:
			alloc(img.width(),img.height(),GL_RGBA8, img.data());
			break;
		default:
//			std::cout << " ... FAILED"<< std::endl;
			return false;
	}
	bind();
	glGenerateMipmap(GL_TEXTURE_2D);
	Texture2D::unbind();
//	std::cout << " ... OK" << std::endl;
	return true;
}

void Texture2D::resize(GLsizei w, GLsizei h)
{
	bind();
	glTexImage2D(GL_TEXTURE_2D, 0, internal_, w, h, 0, external_, data_type_,nullptr);
	width_ = w;
	height_ = h;
	unbind();
}

}
