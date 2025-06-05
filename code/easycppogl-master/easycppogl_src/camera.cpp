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

#include <camera.h>

namespace EZCOGL
{

GLMat4 Camera::perspective(double znear, double zfar) const
{
	double range_inv = 1.0 / (znear - zfar);
	double f = 1.0 / std::tan(field_of_view_ / 2.0);
	auto m05 = (asp_ratio_ > 1) ? std::make_pair(f / asp_ratio_, f) : std::make_pair(f, f * asp_ratio_);
	GLMat4 m;
	m << float(m05.first), 0, 0, 0, 0, float(m05.second), 0, 0, 0, 0, float((znear + zfar) * range_inv),
		float(2 * znear * zfar * range_inv), 0, 0, -1, 0;
	return m;
}

GLMat4 Camera::ortho(double znear, double zfar) const
{
	double range_inv = 1.0 / (znear - zfar);
	auto m05 = (asp_ratio_ < 1) ? std::make_pair(1.0 / asp_ratio_, 1.0) : std::make_pair(1.0, 1.0 / asp_ratio_);
	GLMat4 m;
	m << float(m05.first), 0, 0, 0, 0, float(m05.second), 0, 0, 0, 0, float(2 * range_inv), 0, 0, 0,
		float((znear + zfar) * range_inv), 0;
	return m;
}

Eigen::Matrix4d Camera::perspective_d(double znear, double zfar) const
{
	double range_inv = 1.0 / (znear - zfar);
	double f = 1.0 / std::tan(field_of_view_ / 2.0);
	auto m05 = (asp_ratio_ > 1) ? std::make_pair(f / asp_ratio_, f) : std::make_pair(f, f * asp_ratio_);
	Eigen::Matrix4d m;
	m << m05.first, 0, 0, 0, 0, m05.second, 0, 0, 0, 0, (znear + zfar) * range_inv, 2 * znear * zfar * range_inv, 0, 0,
		-1, 0;
	return m;
}

Eigen::Matrix4d Camera::ortho_d(double znear, double zfar) const
{
	double range_inv = 1.0 / (znear - zfar);
	auto m05 = (asp_ratio_ < 1) ? std::make_pair(1.0 / asp_ratio_, 1.0) : std::make_pair(1.0, 1.0 / asp_ratio_);
	Eigen::Matrix4d m;
	m << m05.first, 0, 0, 0, 0, m05.second, 0, 0, 0, 0, 2 * range_inv, 0, 0, 0, (znear + zfar) * range_inv, 0;
	return m;
}

Eigen::Matrix4d Camera::get_projection_matrix_d() const
{
	double d = focal_dist_ - this->frame_.translation().z();
	znear_ = std::max(0.001 * scene_radius_, d - 1.0 * scene_radius_);
	zfar_ = d + 1.0 * scene_radius_;
	projd_ = ((type_ == PERSPECTIVE) ? perspective_d(znear_, zfar_) : ortho_d(znear_, zfar_));
	return projd_;
}

Eigen::Matrix4d Camera::get_view_matrix_d() const
{

	//  std::cout <<" MV frame "<< std::endl<< this->frame_.matrix() << std::endl;

	Eigen::Affine3d aff =
		Eigen::Translation3d(0.0, 0.0, -focal_dist_) * this->frame_ * Eigen::Translation3d(-pivot_point_);
	Eigen::Matrix4d md = aff.matrix();
	mvd_ = md;
	return mvd_;
}

void Camera::look_dir(const GLVec3& eye, const GLVec3& dir, const GLVec3& up)
{
	Eigen::Affine3d m = Eigen::Affine3d(Transfo::look_dir(eye, dir, up).cast<double>());
	this->frame_ = Eigen::Translation3d(0.0, 0.0, focal_dist_) * m * Eigen::Translation3d(pivot_point_);
}

void Camera::look_info(GLVec3& eye, GLVec3& dir, GLVec3& up)
{
	Eigen::Matrix4d iv = get_view_matrix_d().inverse();
	eye = (iv.block<3, 1>(0, 3)).cast<float>();
	iv.block<3, 1>(0, 3).setZero();
	iv.block<3, 1>(0, 0).normalize();
	iv.block<3, 1>(0, 1).normalize();
	iv.block<3, 1>(0, 2).normalize();
	dir = ((iv*Eigen::Vector4d(0,0,-1,1)).block<3,1>(0,0)).cast<float>();

	double dist = z_far() - scene_radius();
	up = ((iv*Eigen::Vector4d(0,1,0,1)).block<3,1>(0,0)).cast<float>();
}

} // namespace EZCOGL
