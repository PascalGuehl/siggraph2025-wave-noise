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


#ifndef EASY_CPP_OGL_FBO_H_
#define EASY_CPP_OGL_FBO_H_

#include <GL/gl3w.h>
#include <vector>
#include <array>
#include <texture2d.h>

// TODO compatible with TEXUTRE_CUBEMAP ??

namespace EZCOGL
{
class FBO;

class FBO
{
protected:
	FBO(const std::vector<Texture2D::SP>& textures);

public:
	using SP = std::shared_ptr<FBO>;

	static inline FBO::SP create(const std::vector<Texture2D::SP>& textures)
	{
		return std::shared_ptr<FBO>(new FBO(textures));
	}

	~FBO();

	FBO(const FBO&) = delete;

	inline GLint id() const { return id_; }

	inline static void unbind(){};

	void bind();

	static void push();

	static void pop();

	virtual void resize(int w, int h);

	inline GLint width() const { return tex_.front()->width(); }

	inline GLint height() const { return tex_.front()->height(); }

	inline Texture2D::SP texture(std::size_t i) const { return tex_[i]; }

	inline std::size_t nb_textures() const { return tex_.size(); }

	inline virtual Texture2D::SP depth_texture() { return nullptr; }

protected:
	GLuint id_;
	std::vector<Texture2D::SP> tex_;
	std::vector<GLenum> attach_;

	static std::vector<std::array<GLint, 5>> stack_viewport_fbo_;
};


class FBO_Depth : public FBO
{
public:
	using SP = std::shared_ptr<FBO_Depth>;
protected:
	FBO_Depth(const std::vector<Texture2D::SP>& textures, FBO_Depth::SP from = nullptr);
public:
	static inline FBO_Depth::SP create(const std::vector<Texture2D::SP>& textures, FBO_Depth::SP from = nullptr)
	{
		return std::shared_ptr<FBO_Depth>(new FBO_Depth(textures,from));
	}
	~FBO_Depth();
	void resize(int w, int h) override;

protected:
	GLuint depth_render_buffer_;
	bool local_depth_storage_;
};

class FBO_DepthTexture : public FBO
{
protected:
	FBO_DepthTexture(const std::vector<Texture2D::SP>& textures, Texture2D::SP from = nullptr);

	FBO_DepthTexture(const std::vector<Texture2D::SP>& textures, std::shared_ptr<FBO_DepthTexture> from = nullptr);

public:
	using SP = std::shared_ptr<FBO_DepthTexture>;

	static inline FBO_DepthTexture::SP create(const std::vector<Texture2D::SP>& textures, Texture2D::SP from = nullptr)
	{
		return std::shared_ptr<FBO_DepthTexture>(new FBO_DepthTexture(textures,from));
	}

	void resize(int w, int h) override;

	inline Texture2D::SP depth_texture() override { return depth_tex_; }

protected:
	Texture2D::SP depth_tex_;
};

}
#endif
