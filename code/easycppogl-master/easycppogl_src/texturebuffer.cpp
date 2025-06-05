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

#include <texturebuffer.h>
#include <iostream>

namespace EZCOGL
{

TextureBuffer::TextureBuffer(VBO::SP vbo):
		TextureAbstractBuffer() ,vbo_(vbo)
{
	static GLenum internals[]={GL_R32F,GL_RG32F,GL_RGB32F,GL_RGBA32F};

	internal_ = internals[vbo->vector_dimension()];
	glGenTextures(1,&id_);
	glBindTexture(GL_TEXTURE_BUFFER, id_);
	glTexBuffer(GL_TEXTURE_BUFFER, internal_, vbo->id());
	glBindTexture(GL_TEXTURE_BUFFER, 0);

}


TextureBuffer::~TextureBuffer()
{
	glTexBuffer(GL_TEXTURE_BUFFER, internal_, 0);
	glDeleteTextures(1,&id_);
}


TextureUIBuffer::TextureUIBuffer(EBO::SP ebo):
		TextureAbstractBuffer() ,ebo_(ebo)
{
	internal_ = GL_R32UI;
	glGenTextures(1,&id_);
	glBindTexture(GL_TEXTURE_BUFFER, id_);
	glTexBuffer(GL_TEXTURE_BUFFER, internal_, ebo->id());
	glBindTexture(GL_TEXTURE_BUFFER, 0);

}


TextureUIBuffer::~TextureUIBuffer()
{
	glTexBuffer(GL_TEXTURE_BUFFER, internal_, 0);
	glDeleteTextures(1,&id_);
}


}
