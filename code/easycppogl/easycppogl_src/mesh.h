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


#ifndef EASY_CPP_OGL_MESH_H_
#define EASY_CPP_OGL_MESH_H_

#include <gl_eigen.h>
#include <vao.h>
#include <ebo.h>
#include <texture2d.h>
#include <memory>
#include <tuple>
#include <vector>


struct aiMesh;
struct aiMaterial;
struct aiNode;
struct aiScene;

namespace EZCOGL
{


class BoundingBox
{
protected:
	GLVec3 min_;
	GLVec3 max_;
	bool initialized_;
	void direct_add_point(const GLVec3& P);

	inline BoundingBox() :
		initialized_(false)
	{}
public:
	using SP = std::shared_ptr<BoundingBox>;

	inline static BoundingBox::SP create()
	{
		return std::shared_ptr<BoundingBox>(new BoundingBox{});
	}

	void add_point(const GLVec3& P);

	inline GLVec3 center() const
	{
		return (min_+max_)/2.0;
	}

	inline float radius() const
	{
		GLVec3 dv = max_ - min_;
		return dv.norm()/2.0f;
	}

	inline GLMat4 matrix() const
	{
		return Transfo::translate(center()) * Transfo::scale((max_-min_)/2.0f);
	}

	inline const GLVec3& min() const { return min_; }
	inline const GLVec3& max() const { return max_; }

	void merge(const BoundingBox& bb);
};



class Mesh;


class Material
{
public:
	using SP = std::shared_ptr<Material>;
	GLVec3 Ka;
	GLVec3 Kd;
	GLVec3 Ks;
	float Ns;
	float opacity;
	Texture2D::SP tex_ka;
	Texture2D::SP tex_kd;
	Texture2D::SP tex_ks;
	Texture2D::SP tex_opa;
	Texture2D::SP tex_norm_map;
	Texture2D::SP tex_bump_map;

	Material() :
		Ka(0.1f, 0.1f, 0.1f), Kd(0.9f, 0.9f, 0.9f), Ks(1.0f, 1.0f, 1.0f), Ns(250), opacity(1.0f),
		tex_ka(nullptr), tex_kd(nullptr), tex_ks(nullptr), tex_opa(nullptr), tex_norm_map(nullptr), tex_bump_map(nullptr)
	{}
	bool has_kd_texture() const { return tex_kd != nullptr; }
	bool has_ka_texture() const { return tex_ka != nullptr; }
	bool has_ks_texture() const { return tex_ks != nullptr; }
	bool has_opa_texture() const { return tex_opa != nullptr; }
	bool has_norm_texture() const { return tex_norm_map != nullptr; }
	bool has_bump_texture() const { return tex_bump_map != nullptr; }
};

class MeshRenderer
{
	friend class Mesh;
protected:
	VAO::UP vao_;
	GLuint nb_vert_;
	EBO::SP ebo_triangles_;
    EBO::SP ebo_lines_;
    BoundingBox::SP bb_;
	Material::SP mat_;
	MeshRenderer(const Mesh& m, GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col);
public:
	using UP = std::unique_ptr<MeshRenderer>;
	~MeshRenderer();
	MeshRenderer(const MeshRenderer&) = delete;
	void draw(GLenum prim) const ;
    inline const BoundingBox::SP BB() const { return bb_; }
	inline const Material::SP material() const { return mat_; }
	inline bool add_vbos(const std::vector<std::tuple<GLint, VBO::SP>>& att_vbo)
	{
		for( const auto& p: att_vbo)
			if (vao_->use_loc(std::get<0>(p)))
			{
				std::cerr << "Warning location "<< std::get<0>(p) << "already used in this VAO"<< std::endl;
				return false;
			}
		vao_->add(att_vbo);
		return true;
	}

};

class InstancedMeshRenderer
{
	friend class Mesh;
protected:
	VAO::UP vao_;
	GLuint nb_vert_;
	EBO::SP ebo_triangles_;
    EBO::SP ebo_lines_;
    BoundingBox::SP bb_;
	Material::SP mat_;
	InstancedMeshRenderer(const Mesh& m, const std::vector<std::tuple<GLint, VBO::SP, GLint>>& inst_vbos, GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col);
public:
	using UP = std::unique_ptr<InstancedMeshRenderer>;
	~InstancedMeshRenderer();
	InstancedMeshRenderer(const InstancedMeshRenderer&) = delete;
	void draw(GLenum prim, GLuint nb);
	inline const BoundingBox::SP BB() const { return bb_; }
	inline const Material::SP material() const { return mat_; }
};


class Mesh
{
	friend class MeshRenderer;
	friend class InstancedMeshRenderer;
public:
	using SP = std::shared_ptr<Mesh>;
	std::vector<GLVec3> vertices_;
	std::vector<GLVec3> normals_;
	std::vector<GLVec2> tex_coords_;
	std::vector<GLVec3> tangents_;
	std::vector<GLVec3> colors_;
	std::vector<GLuint> tri_indices;
	std::vector<GLuint> line_indices;
	Material::SP mat_;
	std::string path_;
	BoundingBox::SP bb_;
protected:
	inline Mesh()
	{
		bb_ = BoundingBox::create();
		mat_ = std::make_shared<Material>();
	}

	void grid_topo(GLint m, GLint n);

	static void ai_process_node(std::vector<Mesh::SP>& meshes, aiNode *node, const aiScene *scene, const std::string& path, std::map<std::string, Texture2D::SP>& tex_names);

public:
	
	Mesh(::aiMesh* aimesh, ::aiMaterial* aimaterial , const std::string& path, std::map<std::string, Texture2D::SP>& tex_names);
	Mesh(const Mesh&) = delete;
	Mesh(Mesh&& m);

	void compute_normals();

	inline bool has_positions() const { return !vertices_.empty(); }
	inline bool has_tex_coords() const { return !tex_coords_.empty(); }
	inline bool has_normals() const { return !normals_.empty(); }
	inline bool has_tangents() const { return !tangents_.empty(); }
	inline bool has_colors() const { return !colors_.empty(); }

	inline std::size_t nb_vertices() const { return vertices_.size();}

	inline std::vector<GLVec3>& colors() { return colors_ ; }

	inline Material::SP material() { return mat_ ; }
	inline const Material::SP material() const { return mat_; }

	inline const BoundingBox::SP BB() const { return bb_;}

	MeshRenderer::UP renderer(GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col) const;

	InstancedMeshRenderer::UP instanced_renderer(const std::vector<std::tuple<GLint, VBO::SP, GLint>>& inst_vbos,GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col) const;

	static Mesh::SP CubePosOnly();
	static Mesh::SP Cube();
    static Mesh::SP Grid(GLint m=4, GLint n=4);
    static Mesh::SP Wave(GLint m);
	static Mesh::SP Sphere( GLint n);
	//static Mesh::SP ClosedCylinder(int sides, float radius_ratio);
	//static Mesh::SP ClosedCone(int sides, float radius_ratio);
    static Mesh::SP Cylinder(GLint m, GLint n, float radius);
    static Mesh::SP Tore(GLint m, GLint n, float radius_ratio);

	static std::vector<Mesh::SP> load(const std::string& mesh_filename);

	inline std::vector<GLuint>::const_iterator triangle_index(int t) const {return tri_indices.begin()+3*t;}
	inline const GLVec3& triangle_vertex0(std::vector<GLuint>::const_iterator t) const { return vertices_[*t];}
	inline const GLVec3& triangle_vertex1(std::vector<GLuint>::const_iterator t) const { return vertices_[*(t+1)];}
	inline const GLVec3& triangle_vertex2(std::vector<GLuint>::const_iterator t) const { return vertices_[*(t+2)];}

	inline const GLVec3& triangle_normal0(std::vector<GLuint>::const_iterator t) const { return normals_[*t];}
	inline const GLVec3& triangle_normal1(std::vector<GLuint>::const_iterator t) const { return normals_[*(t+1)];}
	inline const GLVec3& triangle_normal2(std::vector<GLuint>::const_iterator t) const { return normals_[*(t+2)];}

	using TRI = std::tuple<uint32_t,uint32_t,uint32_t>;

};



}
#endif
