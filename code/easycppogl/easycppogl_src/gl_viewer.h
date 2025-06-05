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


#ifndef EASY_CPP_OGL_GL_VIEWER_H_
#define EASY_CPP_OGL_GL_VIEWER_H_

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <portable_file_dialogs.h>

#include <camera.h>

namespace EZCOGL
{

class GLViewer
{
protected:
	Camera cam_;
	MovingFrame* current_frame_;
	Eigen::Affine3d inv_cam_;

	double spinning_speed_;
	bool locked_;
	bool need_redraw_;
	double wheel_sensitivity_;
	double mouse_sensitivity_;
	double spin_sensitivity_;
	double double_click_timeout_;
	double last_mouse_x_;
	double last_mouse_y_;
	uint32_t mouse_buttons_;
	bool shift_pressed_;
	bool control_pressed_;
	bool alt_pressed_;
	bool meta_pressed_;
	double last_click_time_;
	int32_t vp_w_;
	int32_t vp_h_;
	GLFWwindow* window_;
	ImGuiContext* context_;
	double time_last_50_frames_;
	double fps_;
	double mem_;
        int avail_gpu_mem_;
	bool show_imgui_;

	void spin();

public:

	GLViewer();

	virtual ~GLViewer();

	int launch2d();

	int launch3d();

	inline bool obj_mode() const { return  current_frame_ != &cam_;}

	inline void ask_update() { need_redraw_ = true; }

        inline Camera& camera() {return cam_;}

	inline GLMat4 get_projection_matrix() const
	{
		return cam_.get_projection_matrix();
	}

	inline GLMat4 get_view_matrix() const
	{
		return cam_.get_view_matrix();
	}

	inline void set_scene_radius(double radius) { cam_.set_scene_radius(radius); }
	inline void set_scene_radius(float radius) { cam_.set_scene_radius(double(radius)); }
	inline void set_scene_radius(int radius) { cam_.set_scene_radius(double(radius)); }
	inline void set_scene_center(const GLVec3& center)
	{
        cam_.set_pivot_point(center);
	}
	inline void set_scene_pivot(const GLVec3& piv) { cam_.change_pivot_point(piv.cast<double>()); }
        inline void center_scene() { cam_.pivot_point(); }
	inline void show_entire_scene() { cam_.show_entire_scene(); }

	void set_size(int w, int h);
	inline int32_t width() const { return vp_w_; }
	inline int32_t height() const { return vp_h_; }
	void update_size();
	inline void lock() { locked_=true; }
	inline void unlock() { locked_=false; }
	inline bool locked() { return locked_; }

	void manip(MovingFrame* fr);

	virtual void mouse_press_ogl(int32_t button, double x, double y);
	virtual void mouse_release_ogl(int32_t button, double x, double y);
	virtual void mouse_dbl_click_ogl(int32_t button, double x, double y);
	virtual void mouse_move_ogl(double x, double y);
	virtual void mouse_wheel_ogl(double x, double y);
	virtual void resize_ogl(int32_t w, int32_t h);
	virtual void close_ogl();
	virtual void init_ogl() = 0;
	virtual void draw_ogl() = 0;
	virtual void interface_ogl();
	virtual void key_press_ogl(int32_t key_code);
	virtual void key_release_ogl(int32_t key_code);
	inline GLFWwindow* window() { return window_; }

	inline void set_wheel_sensitivity(double s) { wheel_sensitivity_ = s*0.005; }
	inline void set_mouse_sensitivity(double s) { mouse_sensitivity_ = s*0.005; }
	inline void set_spin_sensitivity(double s) { spin_sensitivity_ = s*0.025; }
};

float current_time();

}
#endif
