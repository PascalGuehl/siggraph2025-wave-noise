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

#include <mesh.h>
#include <gl_eigen.h>
#include <iostream>
#include <array>
#include <thread>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp>

#include <limits>
#include <cassert>
#include <condition_variable>

class Barrier
{

public:

	Barrier(std::size_t nb_threads)
		: m_mutex(),
		m_condition(),
		m_nb_threads(nb_threads)
	{
		assert(0u != m_nb_threads);
	}

	Barrier(const Barrier& barrier) = delete;

	Barrier(Barrier&& barrier) = delete;

	~Barrier() noexcept
	{
		assert(0u == m_nb_threads);
	}

	Barrier& operator=(const Barrier& barrier) = delete;

	Barrier& operator=(Barrier&& barrier) = delete;

	void Wait()
	{
		std::unique_lock< std::mutex > lock(m_mutex);

		assert(0u != m_nb_threads);

		if (0u == --m_nb_threads)
		{
			m_condition.notify_all();
		}
		else
		{
			m_condition.wait(lock, [this]() { return 0u == m_nb_threads; });
		}
	}

private:

	std::mutex m_mutex;

	std::condition_variable m_condition;

	std::size_t m_nb_threads;
};

#pragma warning( disable : 4244 4245 4018)

namespace EZCOGL
{

const float M_PIF = float(M_PI);

void BoundingBox::direct_add_point(const GLVec3& P)
{
    min_[0] = std::min(min_[0],P[0]);
    min_[1] = std::min(min_[1],P[1]);
    min_[2] = std::min(min_[2],P[2]);
    max_[0] = std::max(max_[0],P[0]);
    max_[1] = std::max(max_[1],P[1]);
    max_[2] = std::max(max_[2],P[2]);
}

void BoundingBox::add_point(const GLVec3& P)
{
	if (!initialized_)
	{
		min_ = P;
		max_ = P;
		initialized_ = true;
	}
	else
		direct_add_point(P);
}

void BoundingBox::merge(const BoundingBox& bb)
{
	if (!initialized_)
	{
		min_ = bb.min_;
		max_ = bb.max_;
		initialized_ = true;
	}
	else
	{
		min_[0] = std::min(min_[0],bb.min_[0]);
		min_[1] = std::min(min_[1],bb.min_[1]);
		min_[2] = std::min(min_[2],min_[2]);
		max_[0] = std::max(max_[0],max_[0]);
		max_[1] = std::max(max_[1],max_[1]);
		max_[2] = std::max(max_[2],max_[2]);
	}
}



Mesh::Mesh(Mesh&& m):
	vertices_(m.vertices_),
	normals_(m.normals_),
	tex_coords_(m.tex_coords_),
	tri_indices(m.tri_indices),
	line_indices(m.line_indices),
	mat_(m.mat_),
	bb_(m.bb_)
{}



void Mesh::compute_normals()
{
	auto old = normals_;
	normals_.clear();
	normals_.resize(vertices_.size(),GLVec3(0,0,0));

	auto nbt = tri_indices.size()/3u;
	for (auto i=0u; i<nbt; ++i)
	{
		const GLVec3& A = vertices_[tri_indices[3*i]];
		const GLVec3& B = vertices_[tri_indices[3*i+1]];
		const GLVec3& C = vertices_[tri_indices[3*i+2]];
		normals_[tri_indices[3 * i]] += (B - A).cross(C - A);
		normals_[tri_indices[3 * i + 1]] += (C - B).cross(A - B);
		normals_[tri_indices[3 * i + 2]] += (A - C).cross(B - C);
	}

	for (auto& n: normals_)
		n.normalize();

	auto it = old.begin();
	auto jt = normals_.begin();

	for (int i=0;i<100;++i)
		std::cout << (*it-*jt).norm() << " =>"  << (it++)->transpose() << " != " << (jt++)->transpose()
				  << std::endl;
		

//	static const int NBTH=16;
//	std::vector<std::thread*> threads;
//	threads.reserve(NBTH);

//	for (int i=0; i<NBTH; ++i)
//	{
//		threads.push_back(new std::thread([this,i] ()
//		{
//			for (int j=i; j<normals_.size(); j+=NBTH)
//			{
//				normals_[j].normalize();
//			}
//		}));
//	}

//	for (auto* t : threads)
//	{
//		t->join();
//		delete t;
//	}
}

MeshRenderer::UP Mesh::renderer(GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col) const
{
	return std::unique_ptr<MeshRenderer>(new  MeshRenderer(*this,att_pos,att_norm,att_tc,att_tang,att_col));
}

MeshRenderer::MeshRenderer(const Mesh& m, GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col):
	bb_(m.bb_),mat_(m.mat_)
{
	nb_vert_ = std::numeric_limits<GLuint>::max();
    std::vector<std::tuple<GLint,VBO::SP>> params;
	if( att_pos>0)
	{
		auto vbop = VBO::create(m.vertices_);
		params.emplace_back(att_pos,vbop);
		nb_vert_ = std::min(size_t(nb_vert_), m.vertices_.size());
	}
	if( att_norm>0)
	{
		auto vbon = VBO::create(m.normals_);
		params.emplace_back(att_norm,vbon);
		nb_vert_ = std::min(size_t(nb_vert_), m.vertices_.size());
	}
	if( att_tc>0)
	{
		auto vbot = VBO::create(m.tex_coords_);
		params.emplace_back(att_tc,vbot);
		nb_vert_ = std::min(size_t(nb_vert_), m.vertices_.size());
	}

	if (att_tang > 0)
	{
		auto vbotg = VBO::create(m.tangents_);
		params.emplace_back(att_tang, vbotg);
		nb_vert_ = std::min(size_t(nb_vert_), m.vertices_.size());
	}

	if( att_col>0)
	{
		auto vboc = VBO::create(m.colors_);
		params.emplace_back(att_col,vboc);
		nb_vert_ = std::min(size_t(nb_vert_), m.vertices_.size());
	}

	vao_ = VAO::create(params);
	ebo_triangles_ = EBO::create(m.tri_indices);
	ebo_lines_ = EBO::create(m.line_indices);
}


MeshRenderer::~MeshRenderer()
{
}

void MeshRenderer::draw(GLenum prim) const
{
	vao_->bind();
	switch (prim)
	{
		case GL_POINTS:
			if (nb_vert_>0)
				glDrawArrays(GL_POINTS, 0, nb_vert_);
			break;
		case GL_LINES:
			if ((ebo_lines_ != nullptr) && (ebo_lines_->length()>0))
			{
				ebo_lines_->bind();
				glDrawElements(GL_LINES, ebo_lines_->length(), GL_UNSIGNED_INT, nullptr);
			}
			break;
		case GL_TRIANGLES:
			if ((ebo_triangles_!= nullptr) && (ebo_triangles_->length()>0))
			{
				ebo_triangles_->bind();
				glDrawElements(GL_TRIANGLES, ebo_triangles_->length(), GL_UNSIGNED_INT, nullptr);
			}
			break;
	}
}


InstancedMeshRenderer::UP Mesh::instanced_renderer(const std::vector<std::tuple<GLint, VBO::SP, GLint>>& inst_vbos, GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col) const
{
	return std::unique_ptr<InstancedMeshRenderer>(new  InstancedMeshRenderer(*this,inst_vbos,att_pos,att_norm,att_tc,att_tang,att_col));
}

InstancedMeshRenderer::InstancedMeshRenderer(const Mesh& m, const std::vector<std::tuple<GLint, VBO::SP, GLint>>& inst_vbos, GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col) :
	bb_(m.bb_), mat_(m.mat_)
{
	std::vector<std::tuple<GLint, VBO::SP, GLint>> params;
	if (att_pos > 0)
	{
		auto vbop = VBO::create(m.vertices_);
		params.emplace_back(att_pos, vbop,0);
		nb_vert_ = std::min(size_t(nb_vert_), m.vertices_.size());
	}
	if (att_norm > 0)
	{
		auto vbon = VBO::create(m.normals_);
		params.emplace_back(att_norm, vbon,0);
		nb_vert_ = std::min(size_t(nb_vert_), m.vertices_.size());
	}
	if (att_tc > 0)
	{
		auto vbot = VBO::create(m.tex_coords_);
		params.emplace_back(att_tc, vbot,0);
		nb_vert_ = std::min(size_t(nb_vert_), m.vertices_.size());
	}


	if (att_tang > 0)
	{
		auto vbotg = VBO::create(m.tangents_);
		params.emplace_back(att_tang, vbotg,0);
		nb_vert_ = std::min(size_t(nb_vert_), m.vertices_.size());
	}

	if (att_col > 0)
	{
		auto vboc = VBO::create(m.colors_);
		params.emplace_back(att_col, vboc,0);
		nb_vert_ = std::min(size_t(nb_vert_), m.vertices_.size());
	}

	for (const auto& v : inst_vbos)
		params.push_back(v);

	vao_ = VAO::create(params);
	ebo_triangles_ = EBO::create(m.tri_indices);
	ebo_lines_ = EBO::create(m.line_indices);
}


InstancedMeshRenderer::~InstancedMeshRenderer()
{
}

void InstancedMeshRenderer::draw(GLenum prim, GLuint nb)
{
	vao_->bind();
	switch (prim)
	{
	case GL_POINTS:
		if (nb_vert_ > 0)
			glDrawArraysInstanced(GL_POINTS, 0, nb_vert_,nb);
		break;
	case GL_LINES:
		if ((ebo_lines_ != nullptr) && (ebo_lines_->length() > 0))
		{
			ebo_lines_->bind();
			glDrawElementsInstanced(GL_LINES, ebo_lines_->length(), GL_UNSIGNED_INT, nullptr,nb);
		}
		break;
	case GL_TRIANGLES:
		if ((ebo_triangles_ != nullptr) && (ebo_triangles_->length() > 0))
		{
			ebo_triangles_->bind();
			glDrawElementsInstanced(GL_TRIANGLES, ebo_triangles_->length(), GL_UNSIGNED_INT, nullptr,nb);
		}
		break;
	}
}


Mesh::SP Mesh::CubePosOnly()
{
	Mesh::SP m{ new Mesh() };

	float V=1.0;
	float v=-1.0;

	m->bb_->add_point(GLVec3(v,v,v));
	m->bb_->add_point(GLVec3(V,V,V));

	m->vertices_ = GLVVec3{{v,v,v}, {V,v,v}, {V,V,v}, {v,V,v}, {v,v,V}, {V,v,V}, {V,V,V}, {v,V,V}};
    m->tri_indices = std::vector<GLuint>{2,1,0,3,2,0, 4,5,6,4,6,7, 0,1,5,0,5,4, 1,2,6,1,6,5, 2,3,7,2,7,6, 3,0,4,3,4,7};
    m->line_indices = std::vector<GLuint>{0,1,1,2,2,3,3,0,4,5,5,6,6,7,7,4,0,4,1,5,2,6,3,7};

	return m;
}

Mesh::SP Mesh::Cube()
{
	Mesh::SP m{ new Mesh() };

	float V=1.0;
	float v=-1.0;

	m->bb_->add_point(GLVec3(v,v,v));
	m->bb_->add_point(GLVec3(V,V,V));

	m->vertices_ = GLVVec3{
			{v,v,v}, {V,v,v}, {V,V,v}, {v,V,v},
			{v,v,V}, {V,v,V}, {V,V,V}, {v,V,V},
			{v,v,V}, {v,v,v}, {v,V,v}, {v,V,V},
			{V,v,V}, {V,v,v}, {V,V,v}, {V,V,V},
			{v,v,V}, {V,v,V}, {V,v,v}, {v,v,v},
			{v,V,V}, {V,V,V}, {V,V,v}, {v,V,v}};

	m->normals_ = GLVVec3{
			{0,0,-1}, {0,0,-1},{0,0,-1},{0,0,-1},
			{0,0,1}, {0,0,1},{0,0,1},{0,0,1},
			{-1,0,0}, {-1,0,0}, {-1,0,0}, {-1,0,0},
			{1,0,0}, {1,0,0}, {1,0,0}, {1,0,0},
			{0,-1,0}, {0,-1,0}, {0,-1,0}, {0,-1,0},
			{0,1,0}, {0,1,0}, {0,1,0}, {0,1,0}};

	m->tex_coords_ = GLVVec2{
		{0,0},{1,0},{1,1},{0,1},
		{0,0},{1,0},{1,1},{0,1},
		{0,0},{1,0},{1,1},{0,1},
		{0,0},{1,0},{1,1},{0,1},
		{0,0},{1,0},{1,1},{0,1},
		{0,0},{1,0},{1,1},{0,1} };

	m->tri_indices = std::vector<GLuint>{0,3,2,0,2,1, 4,5,6,4,6,7, 8,11,10,8,10,9, 12,13,14,12,14,15, 16,19,18,16,18,17, 20,21,22,20,22,23};
	m->line_indices = std::vector<GLuint>{0,1,1,2,2,3,3,0, 4,5,5,6,6,7,7,4, 0,4,1,5,2,6,3,7};

	return m;
}

void Mesh::grid_topo(GLint m, GLint n)
{
	this->tri_indices.reserve(6*(n-1)*(m-1));
	auto push_quad = [&] (GLuint k)
	{
		tri_indices.push_back(k);
		tri_indices.push_back(k-n-1);
		tri_indices.push_back(k-n);

		tri_indices.push_back(k-n-1);
		tri_indices.push_back(k);
		tri_indices.push_back(k-1);
	};

	for(GLint j=1;j<m;++j)
		for(GLint i=1;i<n;++i)
			push_quad(GLuint(j*n+i));

	this->line_indices.reserve(2*m*(n-1)+2*n*(m-1));

	for(GLint j=0;j<m;++j)
		for(GLint i=1;i<n;++i)
		{
			GLint k =j*n+i;
			line_indices.push_back(k);
			line_indices.push_back(k-1);
		}

	for(GLint j=1;j<m;++j)
		for(GLint i=0;i<n;++i)
		{
			GLint k =j*n+i;
			line_indices.push_back(k);
			line_indices.push_back(k-n);
		}
}

Mesh::SP Mesh::Grid(GLint m, GLint n)
{
	Mesh::SP grid{ new Mesh() };
    
    grid->grid_topo(m,n);

	grid->vertices_.reserve(m * n);
	grid->normals_.reserve(grid->vertices_.size());
	grid->tex_coords_.reserve(grid->vertices_.size());
	grid->tangents_.reserve(grid->vertices_.size());

    GLint n1 = n - 1;
    GLint m1 = m - 1;
    if (m>n)
    {
        grid->bb_->add_point({-float(m)/n,-1,-0.01f});
        grid->bb_->add_point({float(m)/n,1,0.01f});
    }
    else
    {
        grid->bb_->add_point({-1,-float(n)/m,-0.01f});
        grid->bb_->add_point({1,float(n)/m,0.01f});
    }

    for(int j=0;j<m;++j)
    {
        float v = (1.0f/m1)*j;
        for(int i=0;i<n;++i)
        {
            float u = (1.0f/n1)*i;

            grid->tex_coords_.push_back(GLVec2(u,v));
            grid->vertices_.push_back(GLVec3(grid->bb_->max().x()*(u-0.5f)*2,grid->bb_->max().y()*(v-0.5f)*2,0.0f));
            grid->normals_.push_back(GLVec3(0,0,1));
			grid->tangents_.push_back(GLVec3(1, 0, 0));
        }
    }
    return grid;
}

Mesh::SP Mesh::Wave(GLint n)
{
	Mesh::SP wave{ new Mesh() };
    
    wave->grid_topo(n,n);
    wave->vertices_.reserve(n * n);
	wave->normals_.reserve(wave->vertices_.size());
	wave->tex_coords_.reserve(wave->vertices_.size());
	wave->tangents_.reserve(wave->vertices_.size());

    GLint n1 = n - 1;
    wave->bb_->add_point({-1,-1,-0.01f});
    wave->bb_->add_point({1,1,0.01f});

    for(int j=0;j<n;++j)
    {
        float v = (1.0f/n1)*j;
        for(int i=0;i<n;++i)
        {
            float u = (1.0f/n1)*i;
            float x = (u-0.5f)*2;
            float y = (v-0.5f)*2;
            float r = std::sqrt(x*x+y*y);
			float h = 0.2f*(1.0f-r/2.0f)*std::sin(float(M_PI)/2+r*8);
			GLVec3 Pos = GLVec3(x, y, h);
            wave->tex_coords_.push_back(GLVec2(u,v));
            wave->vertices_.push_back(Pos);
			float dh = -0.2f/2*std::sin(float(M_PI)/2+r*8) +
					0.2f*(1.0f-r/2)*8*std::cos(float(M_PI)/2+r*8);
            GLVec3 n(-x/r*dh,-y/r*dh,1);
            n.normalize();
            wave->normals_.push_back(n);
            GLVec3 tg = GLVec3(-y, x, 0);
            wave->tangents_.push_back(tg.normalized());
        }
    }
    return wave;
}

Mesh::SP Mesh::Cylinder(GLint m, GLint n, float radius)
{
	Mesh::SP cylinder{ new Mesh() };
    
    cylinder->grid_topo(m,n);

    GLint n1 = n - 1;
    GLint m1 = m - 1;

    cylinder->vertices_.reserve(m*n);
    cylinder->normals_.reserve(cylinder->vertices_.size());
    cylinder->tex_coords_.reserve(cylinder->vertices_.size());
	cylinder->tangents_.reserve(cylinder->vertices_.size());
    GLVVec3 cpos;
    cpos.reserve(n);
    GLVVec3 cnorm;
    cnorm.reserve(n);
    for(int i=0;i<n;++i)
    {
        double alpha = ((1.0/n1)*i)*2*M_PI;
        GLVec3 p(std::sin(alpha),std::cos(alpha),0);
        cnorm.push_back(p);
        cpos.push_back(p*radius);
    }
    for(int j=0;j<m;++j)
    {
        GLMat4 tr = Transfo::translate(GLVec3(0,0,-1.0f+2.0f/m1*j));
        GLMat3 ntr = Transfo::sub33(tr); // no need to inverse_transpose because no scale
        double v = (1.0/n1)*j;
        for(int i=0;i<n;++i)
        {
            double u = (1.0/n1)*i;
            cylinder->tex_coords_.push_back(GLVec2(u,v));
			GLVec3 P = Transfo::apply(tr, cpos[i]);
            cylinder->vertices_.push_back(P);
			GLVec3 N = Transfo::apply(ntr, cnorm[i]);
            cylinder->normals_.push_back(N);
			GLVec3 T = P.cross(GLVec3(0.0f,0.0f,1.0f));
			cylinder->tangents_.push_back(T);
        }
    }

    cylinder->bb_->add_point({-radius,-radius,-1});
    cylinder->bb_->add_point({ radius, radius,1});

    return cylinder;
}


Mesh::SP Mesh::Sphere( GLint n)
{
	Mesh::SP sphere{ new Mesh{} };

	sphere->grid_topo(n,n);

	GLint n1 = n - 1;

	sphere->vertices_.reserve(n*n);
	sphere->normals_.reserve(sphere->vertices_.size());
	sphere->tex_coords_.reserve(sphere->vertices_.size());
	sphere->tangents_.reserve(sphere->vertices_.size());
	for(int j=0;j<n;++j)
	{
		double v = (1.0/n1)*j;
		double beta = ((1.0/n1)*j)*M_PI+M_PI/2;
		float r = std::cos(beta);
		float h = std::sin(beta);
		for(int i=0;i<n;++i)
		{
			double u = (1.0/n1)*i;
			double alpha = ((1.0/n1)*i)*2*M_PI;
			GLVec3 p(r*std::sin(alpha),r*std::cos(alpha),h);
			p.normalize();
			sphere->vertices_.push_back(p);
			sphere->normals_.push_back(p);
            sphere->tex_coords_.push_back(GLVec2(u,v));
            sphere->tangents_.push_back(p.cross(GLVec3(0.0f,0.0f,1.0f)));
		}
	}

	sphere->bb_->add_point({-1,-1,-1});
	sphere->bb_->add_point({ 1,1,1});

	return sphere;
}



Mesh::SP Mesh::Tore(GLint m, GLint n, float radius_ratio)
{
	Mesh::SP tore{ new Mesh{} };

	tore->grid_topo(m,n);

	GLint n1 = n - 1;
	GLint m1 = m - 1;
	float radius0 = 1.0f / (1.0f + radius_ratio);
	float radius1 = radius_ratio * radius0;

    tore->vertices_.reserve(m*n);
    tore->normals_.reserve(tore->vertices_.size());
    tore->tex_coords_.reserve(tore->vertices_.size());
	tore->tangents_.reserve(tore->vertices_.size());

    GLVVec3 cpos;
    cpos.reserve(n);
    GLVVec3 cnorm;	
    cnorm.reserve(n);
	GLVVec3 ctg;
	ctg.reserve(n);
    for(int i=0;i<n;++i)
	{
		double alpha = ((1.0/n1)*i)*2*M_PI;
        GLVec3 p(0,std::sin(alpha),std::cos(alpha));
        cnorm.push_back(p);
		cpos.push_back(p * radius1);
		GLVec3 tg = p.cross(GLVec3(1.0f, 0.0f, 0.0f));
		ctg.push_back(tg);
    }
    for(int j=0;j<m;++j)
    {
        GLMat4 tr = Transfo::rotateZ((360.0/m1)*j)* Transfo::translate(GLVec3(0,radius0,0));
        GLMat3 ntr = Transfo::sub33(tr); // no need to inverse_transpose because no scale
        double v = (1.0/n1)*j;
        for(int i=0;i<n;++i)
        {
            double u = (1.0/n1)*i;
            tore->tex_coords_.push_back(GLVec2(u,v));
            tore->vertices_.push_back(Transfo::apply(tr,cpos[i]));
            tore->normals_.push_back(Transfo::apply(ntr,cnorm[i]));
			tore->tangents_.push_back(Transfo::apply(ntr, ctg[i]));
        }
    }

    tore->bb_->add_point({-1.0f,-1.0f,-radius1});
    tore->bb_->add_point({1.0f,1.0f,radius1});

	return tore;
}

Mesh::Mesh(::aiMesh* aimesh, ::aiMaterial* aimaterial, const std::string& path, std::map<std::string, Texture2D::SP>& tex_names)
{
	bb_ = BoundingBox::create();
	mat_ = std::make_shared<Material>();

	if(aimesh->HasPositions())
	{
		vertices_.reserve(aimesh->mNumVertices);
		for(GLuint i = 0; i < aimesh->mNumVertices; ++i)
		{
			GLVec3 P(aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z);
			bb_->add_point(P);
			vertices_.push_back(P);
		}
//		std::cout << " NB Vertices " << this->vertices_.size() << std::endl;
	}
	if(aimesh->HasNormals())
	{
		normals_.reserve(aimesh->mNumVertices);
		for(GLuint i = 0; i < aimesh->mNumVertices; ++i)
			normals_.push_back(GLVec3(aimesh->mNormals[i].x,aimesh->mNormals[i].y,aimesh->mNormals[i].z));
//		std::cout << " NB Normals " << this->vertices_.size() << std::endl;
	}
	if(aimesh->HasTextureCoords(0))
	{
		tex_coords_.reserve(aimesh->mNumVertices);
		for(GLuint i = 0; i < aimesh->mNumVertices; ++i)
			tex_coords_.push_back(GLVec2(aimesh->mTextureCoords[0][i].x,aimesh->mTextureCoords[0][i].y));
	}

	if (aimesh->HasTangentsAndBitangents())
	{
		tangents_.reserve(aimesh->mNumVertices);
		for (GLuint i = 0; i < aimesh->mNumVertices; ++i)
			tangents_.push_back(GLVec3(aimesh->mTangents[i].x, aimesh->mTangents[i].y, aimesh->mTangents[i].z));
	}

    std::vector<std::vector<GLuint>> accel;

	if(aimesh->HasFaces())
	{
		accel.resize(vertices_.size());

		GLuint nb_tri_ind=aimesh->mNumFaces * 3;
		tri_indices.reserve(nb_tri_ind);

		for(GLuint i = 0; i < aimesh->mNumFaces; ++i)
		{
			auto A = aimesh->mFaces[i].mIndices[0];
			auto B = aimesh->mFaces[i].mIndices[1];
			auto C = aimesh->mFaces[i].mIndices[2];

			tri_indices.push_back(A);
			tri_indices.push_back(B);
			tri_indices.push_back(C);

            if (std::find(accel[B].begin(),accel[B].end(),A) == accel[B].end())
				accel[A].push_back(B);
            if (std::find(accel[C].begin(),accel[C].end(),B) == accel[C].end())
				accel[B].push_back(C);
            if (std::find(accel[A].begin(),accel[A].end(),C) == accel[A].end())
				accel[C].push_back(A);
		}

		line_indices.reserve(nb_tri_ind);
		for (GLuint i=0; i<accel.size(); ++i)
		{
            std::vector<GLuint>& vv = accel[i];
			for (GLuint j=0; j<vv.size(); ++j)
			{
				line_indices.push_back(i);
				line_indices.push_back(vv[j]);
			}
		}
//		std::cout << " NB Triangles " << this->tri_indices.size() / 3 << std::endl;
	}
	else if (this->nb_vertices()>0)
	{	
		GLuint nb_tri_ind = aimesh->mNumFaces * 3;
		tri_indices.reserve(nb_tri_ind);

		for (GLuint i = 0; i < nb_tri_ind; ++i)
		{
			tri_indices.push_back(i);
		}

		line_indices.reserve(2*nb_tri_ind);
		nb_tri_ind /= 3;
		for (GLuint i = 0; i < nb_tri_ind; ++i)
		{
			auto j = 3 * i;
			line_indices.push_back(tri_indices[j++]);
			line_indices.push_back(tri_indices[j]);
			line_indices.push_back(tri_indices[j++]);
			line_indices.push_back(tri_indices[j]);
			line_indices.push_back(tri_indices[j]);
			line_indices.push_back(tri_indices[j-2]);
		}
		std::cout << "WARNING GENERATE FAKE ID TRIANGLE INDICES " << std::endl;
		std::cout << " NB Triangles " << this->tri_indices.size() / 3 << std::endl;
	}

	std::map< aiTextureMapMode, GLenum> mapwrap = { {aiTextureMapMode_Clamp,GL_CLAMP_TO_EDGE},
	{aiTextureMapMode_Decal,GL_CLAMP_TO_BORDER},
	{aiTextureMapMode_Wrap,GL_REPEAT},
	{aiTextureMapMode_Mirror,GL_MIRRORED_REPEAT} };

	auto f_load_tex = [&](aiTextureType tt, Texture2D::SP& tex)
	{
		if (aimaterial->GetTextureCount(tt) > 0)
		{
			aiString fname;
			aiTextureMapMode mm;
			aimaterial->GetTexture(tt, 0, &fname, 0, 0, 0, 0, &mm);
			std::cout << "TEXTURE AI : " << fname.C_Str() << std::endl;
			;
			auto stfn = std::string(fname.C_Str());
			for (auto& c : stfn)
				if (c == '\\')
					c = '/';
			auto it = tex_names.find(stfn);
			if (it == tex_names.end())
			{
				GLenum wm = mapwrap[mm];
				auto t = Texture2D::create({ wm, GL_LINEAR_MIPMAP_LINEAR });
				bool res = t->load(path + "/" + stfn);
				if (!res)
				{
					size_t lp = stfn.find_last_of('.');

					for (size_t i = 0; i < lp; ++i)
					{
						auto& c = stfn[i];
						if ((c >= 'A') && (c <= 'Z'))
							c += 'a' - 'A';
					}
					res = t->load(path + "/" + stfn);
					if (!res)
					{
						for (size_t i = lp + 1; i < stfn.length(); ++i)
						{
							auto& c = stfn[i];
							if ((c >= 'A') && (c <= 'Z'))
								c += 'a' - 'A';
						}
						res = t->load(path + "/" + stfn);
					}
				}
				if (res)
				{
					tex = t;
					tex_names[std::string(fname.C_Str())] = t;
					std::cout << "Loading Texture " << path + "/" + stfn << std::endl;
				}
				else
				{
					std::cerr << "Failed loading Texture " << path + "/" + stfn << std::endl;
				}
			}
			else
			{
				tex = it->second;
				std::cout << "using Texture " << it->first << std::endl;
			}
		}
	};

	// process material
	if (aimesh->mMaterialIndex >= 0)
	{
		
    	aiColor3D color = aiColor3D(0);
		float shininess=0.0f;
		float opacity=1.0f;
    	// Read mtl file vertex data
    	aimaterial->Get(AI_MATKEY_COLOR_AMBIENT, color);
    	mat_->Ka = GLVec3(color.r, color.g, color.b);
		color = aiColor3D(0);
    	aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		mat_->Kd = GLVec3(color.r, color.g, color.b);
		color = aiColor3D(1);
    	aimaterial->Get(AI_MATKEY_COLOR_SPECULAR, color);
		mat_->Ks = GLVec3(color.r, color.g, color.b);
		aimaterial->Get(AI_MATKEY_SHININESS, shininess);
		mat_->Ns = shininess;
		aimaterial->Get(AI_MATKEY_OPACITY, opacity);
		mat_->opacity = opacity;

		//aiString fname;
		//if (aimaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		//{
		//	aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &fname, 0, 0, 0, 0, 0);
		//	auto stfn = std::string(fname.C_Str());
		//	auto it = tex_names.find(stfn);
		//	if ( it == tex_names.end())
		//	{
		//		auto t = Texture2D::create({ GL_MIRRORED_REPEAT,GL_LINEAR_MIPMAP_LINEAR });
		//		tex_names[stfn] = t;
		//		mat_->tex_kd = t;
		//		mat_->tex_kd->load(path + "/" + stfn);
		//	}
		//	else
		//	{
		//		mat_->tex_kd = it->second;
		//	}
		//}
		//if (aimaterial->GetTextureCount(aiTextureType_AMBIENT) > 0)
		//{
		//	aimaterial->GetTexture(aiTextureType_AMBIENT, 0, &fname, 0, 0, 0, 0, 0);
		//	mat_->tex_ka = Texture2D::create({ GL_MIRRORED_REPEAT,GL_LINEAR_MIPMAP_LINEAR });
		//	mat_->tex_ka->load(path + "/" + std::string(fname.C_Str()));
		//}
		//if (aimaterial->GetTextureCount(aiTextureType_SPECULAR) > 0)
		//{
		//	aimaterial->GetTexture(aiTextureType_SPECULAR, 0, &fname, 0, 0, 0, 0, 0);
		//	mat_->tex_ks = Texture2D::create({ GL_MIRRORED_REPEAT,GL_LINEAR_MIPMAP_LINEAR });
		//	mat_->tex_ks->load(path + "/" + std::string(fname.C_Str()));
		//}
		//if (aimaterial->GetTextureCount(aiTextureType_OPACITY) > 0)
		//{
		//	aimaterial->GetTexture(aiTextureType_OPACITY, 0, &fname, 0, 0, 0, 0, 0);
		//	mat_->tex_opa = Texture2D::create({ GL_MIRRORED_REPEAT,GL_LINEAR_MIPMAP_LINEAR });
		//	mat_->tex_opa->load(path + "/" + std::string(fname.C_Str()));
		//}
		//if (aimaterial->GetTextureCount(aiTextureType_NORMALS) > 0)
		//{
		//	aimaterial->GetTexture(aiTextureType_NORMALS, 0, &fname, 0, 0, 0, 0, 0);
		//	mat_->tex_norm_map = Texture2D::create({ GL_MIRRORED_REPEAT,GL_LINEAR_MIPMAP_LINEAR });
		//	mat_->tex_norm_map->load(path + "/" + std::string(fname.C_Str()));
		//}
		//if (aimaterial->GetTextureCount(aiTextureType_HEIGHT) > 0)
		//{
		//	aimaterial->GetTexture(aiTextureType_HEIGHT, 0, &fname, 0, 0, 0, 0, 0);
		//	mat_->tex_bump_map = Texture2D::create({ GL_MIRRORED_REPEAT,GL_LINEAR_MIPMAP_LINEAR });
		//	mat_->tex_bump_map->load(path + "/" + std::string(fname.C_Str()));
		//	std::cout << "BUMP " << path + "/" + std::string(fname.C_Str()) << std::endl;
		//}
		f_load_tex(aiTextureType_DIFFUSE, mat_->tex_kd);
		f_load_tex(aiTextureType_AMBIENT, mat_->tex_ka);
		f_load_tex(aiTextureType_SPECULAR, mat_->tex_ks);
		f_load_tex(aiTextureType_OPACITY, mat_->tex_opa);
		f_load_tex(aiTextureType_NORMALS, mat_->tex_norm_map);
		f_load_tex(aiTextureType_HEIGHT, mat_->tex_bump_map);
	}

}

void Mesh::ai_process_node(std::vector<Mesh::SP>& meshes, aiNode *node, const aiScene *scene, const std::string& path, std::map<std::string, Texture2D::SP>& tex_names)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* aimesh = scene->mMeshes[node->mMeshes[i]];
		aiMaterial* aimaterial = NULL;
		if (aimesh->mMaterialIndex >= 0)
		{
			aimaterial = scene->mMaterials[aimesh->mMaterialIndex];
			auto mn = std::string(aimaterial->GetName().C_Str());
			std::cout << mn << std::endl;
		}
		meshes.push_back(Mesh::SP{new Mesh{aimesh, aimaterial, path, tex_names}});
	}
	for(unsigned int i = 0; i < node->mNumChildren; i++)
	{
		Mesh::ai_process_node(meshes, node->mChildren[i], scene, path, tex_names);
	}
}

/*
void GenMeshTriInd_VertexNormals(aiMesh* pMesh, unsigned int meshIndex)
{
	pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];

	for (unsigned int a = 0; a < pMesh->mNumVertices; a++)
		pMesh->mNormals[a].Set(0, 0, 0);

	for (unsigned int a = 0; a < pMesh->mNumFaces; a++)
	{
		const aiFace& face = pMesh->mFaces[a];
		if (face.mNumIndices == 3)
		{
			const aiVector3D& pV1 = pMesh->mVertices[face.mIndices[0]];
			const aiVector3D& pV2 = pMesh->mVertices[face.mIndices[1]];
			const aiVector3D& pV3 = pMesh->mVertices[face.mIndices[face.mNumIndices - 1]];

			const aiVector3D vNor = ((pV2 - pV1) ^ (pV3 - pV1));

			for (unsigned int i = 0; i < face.mNumIndices; ++i)
				pMesh->mNormals[face.mIndices[i]] += vNor;
		}
		else
		{
			for (unsigned int i = 0; i < face.mNumIndices; ++i)
			{
				const aiVector3D& pA = pMesh->mVertices[face.mIndices[i + face.mNumIndices - 1]];
				const aiVector3D& pB = pMesh->mVertices[face.mIndices[i]];
				const aiVector3D& pC = pMesh->mVertices[face.mIndices[(i + 1) % face.mNumIndices]];

				const aiVector3D vNor = ((pC - pB) ^ (pA - pB));

				for (unsigned int i = 0; i < face.mNumIndices; ++i)
					pMesh->mNormals[face.mIndices[i]] += vNor;
			}
		}
	}
	for (unsigned int a = 0; a < pMesh->mNumVertices; a++)
		pMesh->mNormals[a] = pMesh->mNormals[a].NormalizeSafe();
}
*/

void check_aiVertexNormals(aiNode* node, const aiScene* scene)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* pMesh = scene->mMeshes[node->mMeshes[i]];

		if (!pMesh->HasNormals() && pMesh->HasFaces())
		{
			pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];

			for (unsigned int a = 0; a < pMesh->mNumVertices; a++)
				pMesh->mNormals[a].Set(0, 0, 0);

			for (unsigned int a = 0; a < pMesh->mNumFaces; a++)
			{
				const aiFace& face = pMesh->mFaces[a];
				for (unsigned int i = 0; i < face.mNumIndices; ++i)
				{
					const aiVector3D& pA = pMesh->mVertices[face.mIndices[(i + face.mNumIndices - 1)%face.mNumIndices]];
					const aiVector3D& pB = pMesh->mVertices[face.mIndices[i]];
					const aiVector3D& pC = pMesh->mVertices[face.mIndices[(i + 1) % face.mNumIndices]];
					aiVector3D BC = (pC - pB).Normalize();
					aiVector3D BA = (pA - pB).Normalize();
					//const aiVector3D vNor = (BC ^ BA);
					 float area = (BC ^ BA).Length();
					 if (area > 1.0)
						area = 3.1415926f-std::asin(1.0f-area);
					 else
						area = std::asin(area);
					 const aiVector3D vNor = (BC ^ BA).NormalizeSafe() * area;
					pMesh->mNormals[face.mIndices[i]] += vNor;
				}
			}
			for (unsigned int a = 0; a < pMesh->mNumVertices; a++)
				pMesh->mNormals[a] = pMesh->mNormals[a].NormalizeSafe();
		}
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		check_aiVertexNormals(node->mChildren[i], scene);
	}
}



std::vector<Mesh::SP> Mesh::load(const std::string& mesh_filename)
{

	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
	Assimp::Importer importer;
	
	const aiScene* scene = importer.ReadFile(mesh_filename, aiProcess_Triangulate);
	check_aiVertexNormals(scene->mRootNode, scene);
	scene = importer.ApplyPostProcessing(aiProcess_CalcTangentSpace |aiProcess_GenUVCoords);
	
	if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
         return std::vector<Mesh::SP>();
	 }

	auto mfn = mesh_filename;
	for (auto& c: mfn)
		if (c=='\\')
			c='/';

	std::string dir = mfn.substr(0, mfn.find_last_of('/'));

	std::map<std::string, Texture2D::SP> tex_names; // for unique file loading
     std::vector<Mesh::SP> meshes;
	 Mesh::ai_process_node(meshes, scene->mRootNode, scene, dir, tex_names);

	 return meshes;
}




}
