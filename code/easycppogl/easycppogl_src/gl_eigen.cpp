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

#include "gl_eigen.h"

namespace EZCOGL
{

namespace Transfo {

GLMat4 translate(const GLVec3& v)
{
	GLMat4 m;
	m.setIdentity();
	m(0,3) = v.x();
	m(1,3) = v.y();
	m(2,3) = v.z();
	return m;
}

GLMat4 translate(float x, float y, float z)
{
	GLMat4 m;
	m.setIdentity();
	m(0,3) = x;
	m(1,3) = y;
	m(2,3) = z;
	return m;
}

GLMat4 scale(float s)
{
	GLMat4 m;
	m.setIdentity();
	m(0,0) = s;
	m(1,1) = s;
	m(2,2) = s;
	return m;
}


GLMat4 scale(float sx, float sy, float sz)
{
	GLMat4 m;
	m.setIdentity();
	m(0,0) = sx;
	m(1,1) = sy;
	m(2,2) = sz;
	return m;
}

GLMat4 scale(const GLVec3 & sv)
{
	GLMat4 m;
	m.setIdentity();
	m(0,0) = sv.x();
	m(1,1) = sv.y();
	m(2,2) = sv.z();
	return m;
}


GLMat4 rotateX(float a)
{
	float alpha = float(M_PI / 180) * a;
	float s = std::sin(alpha);
	float c = std::cos(alpha);
	GLMat4 m;
	m.setIdentity();
	m(1,1) = c;
	m(2,1) = s;
	m(1,2) = -s;
	m(2,2) = c;
	return m;
}

GLMat4 rotateY(float a)
{
	float alpha = float(M_PI / 180) * a;
	float s = std::sin(alpha);
	float c = std::cos(alpha);
	GLMat4 m;
	m.setIdentity();
	m(0,0) = c;
	m(2,0) = -s;
	m(0,2) = s;
	m(2,2) = c;
	return m;
}

GLMat4 rotateZ(float a)
{
	float alpha = float(M_PI / 180) * a;
	float s = std::sin(alpha);
	float c = std::cos(alpha);
	GLMat4 m;
	m.setIdentity();
	m(0,0) = c;
	m(1,0) = s;
	m(0,1) = -s;
	m(1,1) = c;
	return m;
}

GLMat4 rotate(float a, const GLVec3& axis)
{
	float alpha = float(M_PI / 180) * a;
	return Eigen::Transform<float,3,Eigen::Affine>(Eigen::AngleAxisf(alpha,axis)).matrix();
}


GLMat4 look_dir(const GLVec3& eye, const GLVec3& dir, const GLVec3& up)
{
  GLVec3 zAxis = -(dir.normalized());
  GLVec3 xAxis = up.normalized().cross(zAxis).normalized();
  GLVec3 yAxis = zAxis.cross(xAxis).normalized();

  GLMat4 m;
  m.block<1,3>(0, 0) = xAxis.transpose();
  m.block<1,3>(1, 0) = yAxis.transpose();
  m.block<1,3>(2, 0) = zAxis.transpose();
  m.block<3,1>(0, 3) = GLVec3(-xAxis.dot(eye), -yAxis.dot(eye), -zAxis.dot(eye));
  m.block<1,4>(3,0) = GLVec4(0,0,0,1).transpose();

  return m;
}






GLMat4 perspective(float ifov2, float aspect, float znear, float zfar)
{
	GLMat4 m;
	float rangeInv = 1.0f/(znear-zfar);
	if (aspect>1)
	{
		m(0,0) = ifov2/aspect;
		m(1,1) = ifov2;
	}
	else
	{
		m(0,0) = ifov2;
		m(1,1) = ifov2*aspect;
	}

				m(0,1) = 0; m(0,2) = 0;	m(0,3) = 0;
	m(1,0) = 0;				m(1,2) = 0;	m(1,3) = 0;
	m(2,0) = 0;	m(2,1) = 0; m(2,2) = (znear+zfar)*rangeInv; m(2,3) = -1.0;
	m(3,0) = 0;	m(3,1) = 0; m(3,2) = 2*znear*zfar*rangeInv; m(3,3) = 0;

	return m;
}

GLMat4 ortho(float half_width, float half_height, float znear, float zfar)
{
	GLMat4 m;
	float rangeInv = 1.0f / (znear - zfar);

	m(0,0) = 1.0f/half_width;
	m(1,1) = 1.0f/half_height;

				m(0,1) = 0; m(0,2) = 0;	m(0,3) = 0;
	m(1,0) = 0;				m(1,2) = 0;	m(1,3) = 0;
	m(2,0) = 0;	m(2,1) = 0; m(2,2) = 2*rangeInv;			m(2,3) = 0.0;
	m(3,0) = 0;	m(3,1) = 0; m(3,2) = (znear+zfar)*rangeInv;	m(3,3) = 1;

	return m;
}

GLMat4 ortho2D(float width, float height)
{
	GLMat4 m;
	m.setIdentity();
	float aspect = width / height;
	m(0,0) = aspect<1 ? 1/aspect : 1;
	m(1,1) = aspect<1 ? 1 : aspect;
	return m;
}


GLMat2 ortho2D_2(float width, float height)
{
	GLMat2 m;
	m.setIdentity();
	float aspect = width / height;
	m(0,0) = aspect<1 ? 1/aspect : 1;
	m(1,1) = aspect<1 ? 1 : aspect;
	return m;
}

} //end namespace Transfo

/**
 * @brief look_dir
 * @param eye
 * @param dir
 * @param up
 * @return
 * TODO CHECK ????
 */


} // namespace
