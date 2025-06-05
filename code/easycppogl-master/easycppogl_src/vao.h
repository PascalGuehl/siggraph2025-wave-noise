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

#ifndef EASY_CPP_OGL_VAO_H_
#define EASY_CPP_OGL_VAO_H_

#include <GL/gl3w.h>
#include <vbo.h>
#include <tuple>
#include <iostream>

namespace EZCOGL
{

class VAO
{
	VAO(const std::vector<std::tuple<GLint,VBO::SP>>& att_vbo);

	VAO(const std::vector<std::tuple<GLint,VBO::SP,GLint>>& att_vbo);

	VAO(const std::vector<std::tuple<GLint,VBO::SP,GLint,GLint,GLint>>& att_vbo);

public:
	using UP = std::unique_ptr<VAO>;

	VAO(const VAO&) = delete ;
	~VAO();
	static const VAO::UP& none();
	static void bind_none();

	bool use_loc(GLuint l)
	{
		return vbos_.find(l) != vbos_.end();
	}

	void add(const std::vector<std::tuple<GLint, VBO::SP>>& att_vbo);

	void add(const std::vector<std::tuple<GLint, VBO::SP, GLint>>& att_vbo);

	void add(const std::vector<std::tuple<GLint, VBO::SP, GLint, GLint, GLint>>& att_vbo);

	/**
	* @brief remove vbo ref from VBO
	* 
	*/
	void remove(GLint att_loc, VBO::SP vbo);

	/**
	 * @brief create VAO
	 * @param att_vbo couple attribute id, VBO
	 * @return
	 */
	inline static VAO::UP create(const std::vector<std::tuple<GLint,VBO::SP>>& att_vbo)
	{
		//return std::make_unique<VAO>(att_vbo);
		return std::unique_ptr<VAO>(new VAO(att_vbo));

	}

	/**
	 * @brief create VAO for instancing
	 * @param att_vbo triplet attribute id, VBO, instancing divisor (read the OGL doc!)
	 * @return
	 */
	inline static VAO::UP create(const std::vector<std::tuple<GLint,VBO::SP, GLint>>& att_vbo)
	{
		//return std::make_unique<VAO>(att_vbo);
		return std::unique_ptr<VAO>(new VAO(att_vbo));
	}

	/**
	 * @brief create with all param (do not use, expert only)
	 * @param att_vbo
	 * @return
	 */
	inline static VAO::UP create(const std::vector<std::tuple<GLint,VBO::SP, GLint, GLint, GLint>>& att_vbo)
	{
		//return std::make_unique<VAO>(att_vbo);
		return std::unique_ptr<VAO>(new VAO(att_vbo));
	}


	inline void bind() const
	{
		glBindVertexArray(id_);
	}

	static inline void unbind()
	{
		glBindVertexArray(0);
	}



protected:
	GLuint id_;
	GLuint nb_;
	std::map<GLint, VBO::SP> vbos_; // map  :key att loc, data VBO, divisor
	static VAO::UP none_;

	template <typename ...OTHERS>
	void store_vbo(const std::tuple<GLint, VBO::SP, OTHERS...>& p )
	{
		auto ret = vbos_.insert(std::make_pair(std::get<0>(p), std::get<1>(p)));
		if (!ret.second)
			std::cerr << "Warning attribute location " << std::get<0>(p) << "already used " << std::endl;
	}
};

} // namespace

#endif // EASY_CPP_OGL_VAO_H_
