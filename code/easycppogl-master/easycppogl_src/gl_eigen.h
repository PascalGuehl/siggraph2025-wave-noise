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

#ifndef EASY_CPP_OGL_GL_EIGEN_H_
#define EASY_CPP_OGL_GL_EIGEN_H_


#include <cmath>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Eigen>
#include <Eigen/Geometry>
#include <Eigen/SVD>
#include <vector>
#include <type_traits>

namespace is_eigen_detail
{
	template <typename T>
	std::true_type test(const Eigen::MatrixBase<T>*);
	std::false_type test(...);
}
template <typename T>
struct is_eigen : public decltype(is_eigen_detail::test(std::declval<T*>()))
{};

template <typename T>
int32_t SIZE32(const T& x)
{
	return int32_t(x.size());
}
template <typename T>
uint32_t USIZE32(const T& x)
{
	return uint32_t(x.size());
}

namespace EZCOGL
{
using GLVec2 = Eigen::Vector2f;
using GLVec3 = Eigen::Vector3f;
using GLVec4 = Eigen::Vector4f;
using GLMat2 = Eigen::Matrix2f;
using GLMat3 = Eigen::Matrix3f;
using GLMat4 = Eigen::Matrix4f;
using GLColor = Eigen::Vector4f;

using GLVVec2 = std::vector<GLVec2>;
using GLVVec3 = std::vector<GLVec3>;
using GLVVec4 = std::vector<GLVec4>;

namespace Transfo
{
GLMat4 translate(const GLVec3& v);
GLMat4 translate(float x, float y, float z);
GLMat4 scale(float s);
GLMat4 scale(float sx, float sy, float sz);
GLMat4 scale(const GLVec3& sv);
GLMat4 rotateX(float a);
GLMat4 rotateY(float a);
GLMat4 rotateZ(float a);
GLMat4 rotate(float a, const GLVec3& axis);


inline GLMat3 inverse_transpose(const GLMat4& mv)
{
        return mv.inverse().transpose().block<3,3>(0,0);
}

inline GLVec3 apply(const GLMat4& tr, const GLVec3& p)
{
        return (tr*GLVec4(p.x(),p.y(),p.z(),1.0f)).block<3,1>(0,0);
}

inline GLVec3 applyproj(const GLMat4& tr, const GLVec3& p)
{
        GLVec4 P4 = tr*GLVec4(p.x(),p.y(),p.z(),1.0f);

        return P4.block<3,1>(0,0) / P4.w();
}


inline GLVec3 apply(const GLMat3& tr, const GLVec3& p)
{
        return tr*p;
}


inline GLMat3 sub33(const GLMat4& m)
{
        return m.block<3,3>(0,0);
}

/**
 * @brief look_dir
 * @param eye
 * @param dir
 * @param up
 * @return
 */
GLMat4 look_dir(const GLVec3& eye, const GLVec3& dir, const GLVec3& up);

//inline GLMat4 look_at(const GLVec3& eye, const GLVec3& at, const GLVec3& up)
//{
//  return look_dir(eye, (at - eye).eval(), up);
//}

template <typename D1, typename D2>
inline D1 mix(const Eigen::MatrixBase<D1>& A, const Eigen::MatrixBase<D2>& B,
              float k)  // -> typename std::enable_if<is_eigen<T1>::value &&
                        // is_eigen<T2>::value,T1::>::type
{
  return (1.0f - k) * A + k * B;
}

inline float mix(float A, float B, float k)
{
	return (1.0f-k)*A+ k*B;
}

inline double mix(double A, double B, double k)
{
	return (1.0-k)*A+ k*B;
}


//inline GLVec3 reflect(const GLVec3& I, const GLVec3& N)
template <typename D1,typename D2>
inline D1 reflect(const Eigen::MatrixBase<D1>& I, const Eigen::MatrixBase<D2>& N)
{
	return I - 2.0f * N.dot(I) * N;
//	float k = N.dot(-I);
//	return I + (2.0f*k) * N;
}

//inline GLVec3 refract(const GLVec3& I, const GLVec3& N, float ratio)
template <typename D1,typename D2>
inline D1 refract(const Eigen::MatrixBase<D1>& I, const Eigen::MatrixBase<D2>& N, float ratio)
{
	float r2 = ratio*ratio;
	float k = N.dot(-I);
	float kk2 = 1.0f - r2 * (1.0f - k*k);
	if (kk2<0)
		return reflect(I,N);
	float kk = std::sqrt(kk2);

	if (k>=0)
		return (ratio*I + (ratio*k-kk)*N).normalized();
	else
		return (ratio*I - (ratio*k+kk)*N).normalized();
}


GLMat4 perspective(float ifov2, float aspect, float znear, float zfar);

GLMat4 ortho(float half_width, float half_height, float znear, float zfar);

GLMat4 ortho2D(float width, float height);

GLMat2 ortho2D_2(float width, float height);

template<typename T>
inline float max_index_compo(const T& vec)
{
	int m =0;
	for (int i=1; i<T::RowsAtCompileTime; i++)
	{
		if (vec[i] > vec[m])
			m = i;
	}
	return m;
}


template<typename T>
inline float max_compo(const T& vec)
{
	return vec[max_index_compo(vec)];
}

template<typename T>
inline float min_index_compo(const T& vec)
{
	int m =0;
	for (int i=1; i<T::RowsAtCompileTime; i++)
	{
		if (vec[i] < vec[m])
			m = i;
	}
	return m;
}


template<typename T>
inline float min_compo(const T& vec)
{
	return vec[min_index_compo(vec)];
}

} // namespace Transfo

} // namespace EZCOGL

#endif //
