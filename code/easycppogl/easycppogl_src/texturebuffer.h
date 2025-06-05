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

#ifndef EASY_CPP_OGL_TEXTURE_BUFFER_H_
#define EASY_CPP_OGL_TEXTURE_BUFFERH_

#include <GL/gl3w.h>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <vbo.h>
#include <ebo.h>

namespace EZCOGL
{




class TextureAbstractBuffer
{
protected:
	GLuint id_;
	GLenum internal_;

public:

	inline TextureAbstractBuffer() {}
	inline virtual ~TextureAbstractBuffer() {}

	TextureAbstractBuffer(const TextureAbstractBuffer&) = delete ;

	inline GLuint id()
	{
		return id_;
	}


	inline void bind()
	{
		glBindTexture(GL_TEXTURE_BUFFER, id_);
	}

	inline GLint bind(GLint unit)
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_BUFFER, id_);
		return unit;
	}

	inline static void unbind()
	{
		glBindTexture(GL_TEXTURE_BUFFER,0);
	}
};

class TextureBuffer;
using SP_TextureBuffer = std::shared_ptr<TextureBuffer>;

class TextureBuffer :public TextureAbstractBuffer
{
protected:
	VBO::SP vbo_;

public:
	TextureBuffer(VBO::SP vbo);
	~TextureBuffer();

	static SP_TextureBuffer create(VBO::SP vbo)
	{
		return std::make_shared<TextureBuffer>(vbo);
	}

	TextureBuffer(const TextureBuffer&) = delete ;

	inline VBO& vbo()
	{
		return *vbo_;
	}
};

class TextureUIBuffer;
using SP_TextureUIBuffer = std::shared_ptr<TextureUIBuffer>;

class TextureUIBuffer :public TextureAbstractBuffer
{
protected:
	EBO::SP ebo_;

public:
	TextureUIBuffer(EBO::SP ebo);
	~TextureUIBuffer();

	static SP_TextureUIBuffer create(EBO::SP ebo)
	{
		return std::make_shared<TextureUIBuffer>(ebo);
	}

	TextureUIBuffer(const TextureUIBuffer&) = delete ;

	inline EBO& ebo()
	{
		return *ebo_;
	}
};


}
#endif
