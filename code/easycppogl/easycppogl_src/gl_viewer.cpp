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

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <gl_viewer.h>
#include <fbo.h>

#include <chrono>
#include <thread>

extern bool Uniform_Explicit_Location_Support;


namespace EZCOGL
{

static void glfw_error_callback(int error, const char* description)
{
	std::cerr <<"Glfw Error "<<error << ": " << description << std::endl;
}

void GLViewer::set_size(int w, int h)
{
	glfwSetWindowSize(window_,w,h);
}

GLViewer::GLViewer():
	locked_(true),
	need_redraw_(true),
	wheel_sensitivity_(0.0005),
	mouse_sensitivity_(0.005),
	spin_sensitivity_(0.025),
	double_click_timeout_(0.3),
	last_mouse_x_(0),
	last_mouse_y_(0),
	mouse_buttons_(0),
	shift_pressed_(false),
	control_pressed_(false),
	alt_pressed_(false),
	meta_pressed_(false),
	last_click_time_(0),
	vp_w_(0),
	vp_h_(0),
	show_imgui_(true)
{
	current_frame_ = &cam_;

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GFLW!" << std::endl;
	}

	const char* glsl_version = "#version 330";
#ifdef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#endif

	window_ = glfwCreateWindow(720, 720, "EOGL", nullptr, nullptr);
	if (window_ == nullptr)
	{
		std::cerr << "Failed to create Window!" << std::endl;
	}

	glfwMakeContextCurrent(window_);

	bool err = gl3wInit() != 0;
	if (err)
	{
		std::cerr << "Failed to initialize OpenGL loader!" << std::endl;
	}
	glfwSwapInterval(1); // Enable vsync

	IMGUI_CHECKVERSION();
	context_ = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

#ifdef _WIN32
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	float xscale, yscale;
	glfwGetMonitorContentScale(monitor, &xscale, &yscale);
	if (xscale > 1 || yscale > 1)
	{
		float highDPIscaleFactor = xscale;
		ImGuiStyle &style = ImGui::GetStyle();
		style.ScaleAllSizes(highDPIscaleFactor);
		ImGui::GetIO().FontGlobalScale = highDPIscaleFactor;
	}
#endif

	ImGui::StyleColorsDark();//ImGui::StyleColorsClassic();
	ImGui_ImplGlfw_InitForOpenGL(window_, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	glfwSetWindowUserPointer(window_,this);
	glfwGetCursorPos(window_,&last_mouse_x_,&last_mouse_y_);

	std::cout << glGetString(GL_VENDOR)<< std::endl;
	std::cout << glGetString(GL_RENDERER)<< std::endl;
	std::cout << glGetString(GL_VERSION)<< std::endl;

	int nb_ext;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nb_ext);
	Uniform_Explicit_Location_Support = false;
	for (int i=0; i <nb_ext; ++i)
	{
		const char* name = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
		if (std::string(name) == "GL_ARB_explicit_uniform_location")
			Uniform_Explicit_Location_Support = true;
	}

	glfwSetWindowSizeCallback(window_, [](GLFWwindow* wi, int , int)
	{
		GLViewer* that = static_cast<GLViewer*>(glfwGetWindowUserPointer(wi));
		glfwGetFramebufferSize(wi, &(that->vp_w_), &(that->vp_h_));
		that->need_redraw_ = true;
		that->cam_.set_aspect_ratio(double(that->vp_w_)/that->vp_h_);
		if (!that->locked())
			that->resize_ogl(that->vp_w_,that->vp_h_);
		glViewport(0,0,that->vp_w_,that->vp_h_);
	});

	glfwSetMouseButtonCallback(window_, [](GLFWwindow* wi, int b, int a, int m)
	{
		double cx,cy;
		glfwGetCursorPos(wi,&cx,&(cy));
		ImGui::GetIO().MousePos = ImVec2(GLfloat(cx),GLfloat(cy));

		GLViewer* that = static_cast<GLViewer*>(glfwGetWindowUserPointer(wi));
		if (ImGui::GetIO().WantCaptureMouse || ImGui::IsMouseHoveringAnyWindow())
		{
			that->mouse_buttons_ = 0;
			return;
		}

		that->shift_pressed_   = (m & GLFW_MOD_SHIFT);
		that->control_pressed_ = (m & GLFW_MOD_CONTROL);
		that->alt_pressed_     = (m & GLFW_MOD_ALT);
		that->meta_pressed_    = (m & GLFW_MOD_SUPER);;
		that->last_mouse_x_ = cx;
		that->last_mouse_y_ = cy;
		double now = glfwGetTime();
		switch(a)
		{
			case GLFW_PRESS:
			that->mouse_buttons_ |= 1<<b;
			that->mouse_press_ogl(b,that->last_mouse_x_,that->last_mouse_y_);
			if (now - that->last_click_time_ < that->double_click_timeout_)
				that->mouse_dbl_click_ogl(b,that->last_mouse_x_,that->last_mouse_y_);
			that->last_click_time_ = now;
			break;
			case GLFW_RELEASE:
			that->mouse_buttons_ &= ~(1<<b);
			that->mouse_release_ogl(b,that->last_mouse_x_,that->last_mouse_y_);
			break;
		}
	});

	glfwSetScrollCallback(window_, [](GLFWwindow* wi, double dx, double dy)
	{
		GLViewer* that = static_cast<GLViewer*>(glfwGetWindowUserPointer(wi));
		if (ImGui::GetIO().WantCaptureMouse || ImGui::IsMouseHoveringAnyWindow())
		{
			that->mouse_buttons_ = 0;
			return;
		}
		double cx,cy;
		glfwGetCursorPos(wi,&cx,&(cy));
		that->mouse_wheel_ogl(dx,100.0*dy);
	});


	glfwSetCursorPosCallback(window_, [](GLFWwindow* wi, double x, double y)
	{
		GLViewer* that = static_cast<GLViewer*>(glfwGetWindowUserPointer(wi));
		if (ImGui::GetIO().WantCaptureMouse || ImGui::IsMouseHoveringAnyWindow())
		{
			that->mouse_buttons_ = 0;
			return;
		}

		double cx,cy;
		glfwGetCursorPos(wi,&cx,&(cy));
		ImGui::GetIO().MousePos = ImVec2(float(cx),float(cy));

		that->mouse_move_ogl(x,y);
	});

	glfwSetKeyCallback(window_, [](GLFWwindow* wi, int k, int /*s*/, int a, int m)
	{
		double cx,cy;
		glfwGetCursorPos(wi,&cx,&(cy));
		GLViewer* that = static_cast<GLViewer*>(glfwGetWindowUserPointer(wi));

		if (k==GLFW_KEY_ESCAPE)
			exit(0);

		that->shift_pressed_   = (m & GLFW_MOD_SHIFT);
		that->control_pressed_ = (m & GLFW_MOD_CONTROL);
		that->alt_pressed_     = (m & GLFW_MOD_ALT);
		that->meta_pressed_    = (m & GLFW_MOD_SUPER);;
		switch(a)
		{
		case GLFW_PRESS:
			that->key_press_ogl(k);
			break;
		case GLFW_RELEASE:
			that->key_release_ogl(k);
			break;
		}
	});

}

GLViewer::~GLViewer()
{}

void GLViewer::update_size()
{
	glfwGetFramebufferSize (window(), &vp_w_, &vp_h_);
}

void GLViewer::manip(MovingFrame* fr)
{
	if (fr != nullptr)
	{
		current_frame_ = fr;
		inv_cam_ = cam_.frame_.inverse();
	}
	else
	{
		current_frame_ = &(cam_);
	}
}

void GLViewer::resize_ogl(int32_t, int32_t)
{}


void GLViewer::mouse_press_ogl(int32_t button, double , double )
{
	if (locked_)
		return;
	if (button == 0)
	{
		current_frame_->is_moving_ = false;
		spinning_speed_ = 0;
		current_frame_->spin_ = Eigen::Affine3d::Identity();
		need_redraw_ = true;
	}
}

void GLViewer::mouse_release_ogl(int32_t button, double , double )
{
	if (locked_)
		return;
	if (button == 0)
	{
		current_frame_->is_moving_ = (spinning_speed_ > spin_sensitivity_);
		if (! current_frame_->is_moving_)
		{
			spinning_speed_ = 0;
			current_frame_->spin_ = Eigen::Affine3d::Identity();
		}
		need_redraw_ = true;
	}
}


void GLViewer::mouse_move_ogl(double x, double y)
{
	if (locked_)
		return;

	double dx = x - last_mouse_x_;
	double dy = y - last_mouse_y_;

	last_mouse_x_ = x;
	last_mouse_y_ = y;

	if ((mouse_buttons_ & 1) && ((std::abs(dx)+ std::abs(dy))>0.0))
	{
		Eigen::Vector3d axis1(double(dy),double(dx),0);
		spinning_speed_ = axis1.norm();
		axis1 /= spinning_speed_;
		spinning_speed_ *= mouse_sensitivity_;

		Eigen::Vector3d u(x / width() - 0.5, 0.5 - y / height(), 0);
		double s = 2.0 * u.dot(axis1);
		double n = -1.0;
		if (s < 0.0)
		{
			s = -s;
			n = -n;
		}
		Eigen::Vector3d axis = ((1.0-s)*axis1 + s * Eigen::Vector3d(0, 0, n)).normalized();


		current_frame_->spin_ = Eigen::AngleAxisd(0.2*spinning_speed_,axis);
		auto tr = current_frame_->frame_.translation().eval();
		current_frame_->frame_.translation().setZero();
		current_frame_->frame_ = Eigen::AngleAxisd(spinning_speed_,axis) * current_frame_->frame_;
		current_frame_->frame_.translation() = tr;
		need_redraw_ = true;
	}
	
	if (mouse_buttons_ & 2)
	{
		double zcam = 1.0/std::tan(cam_.field_of_view()/2.0);
		double a = cam_.scene_radius() - cam_.frame_.translation().z()/ zcam;
		double nx = dx / vp_w_ * cam_.width() * a;
		double ny = -dy / vp_h_ * cam_.height() * a;
		cam_.frame_.translation().x() += 2*nx;
		cam_.frame_.translation().y() += 2*ny;
		need_redraw_ = true;
	}
}


void GLViewer::spin()
{
	if (locked_)
		return;

	if (current_frame_->is_moving_)
	{
		auto tr = current_frame_->frame_.translation().eval();
		current_frame_->frame_.translation().setZero();
		current_frame_->frame_ = current_frame_->spin_ * current_frame_->frame_;
		current_frame_->frame_.translation() = tr;
		need_redraw_ = true;
	}
}


void GLViewer::mouse_dbl_click_ogl(int32_t, double, double)
{
}


void GLViewer::mouse_wheel_ogl(double , double dy)
{
	if (locked_)
		return;

	if (dy!=0.0)
	{
		double zcam = 1.0/std::tan(cam_.field_of_view()/2.0);
		double a = cam_.scene_radius() - cam_.frame_.translation().z()/zcam/cam_.scene_radius();
		cam_.frame_.translation().z() -= wheel_sensitivity_*dy*std::max(0.1,a);
		need_redraw_=true;
	}
}


void GLViewer::key_press_ogl(int32_t /*key_code*/)
{}

void GLViewer::key_release_ogl(int32_t /*key_code*/)
{}

void GLViewer::close_ogl()
{}

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

#define GL_VBO_FREE_MEMORY_ATI                     0x87FB
#define GL_TEXTURE_FREE_MEMORY_ATI                 0x87FC
#define GL_RENDERBUFFER_FREE_MEMORY_ATI            0x87FD

int GLViewer::launch2d()
{
	locked_ = false;
	init_ogl();

	glfwGetFramebufferSize(window_, &vp_w_, &vp_h_);
	need_redraw_ = true;
	cam_.set_aspect_ratio(double(vp_w_)/vp_h_);
	resize_ogl(vp_w_,vp_h_);
	glViewport(0,0,vp_w_,vp_h_);

	int32_t frame_counter = 0;
	while (!glfwWindowShouldClose(window_))
	{
		if (++frame_counter == 50)
		{
			double now = glfwGetTime();
			frame_counter = 0;
			fps_ = 50 / (now - time_last_50_frames_);
			time_last_50_frames_ = now;
		}


		glfwPollEvents();
		glfwMakeContextCurrent(window_);
		this->draw_ogl();
		if (show_imgui_)
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			this->interface_ogl();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
		glfwSwapBuffers(window_);
	}
	glfwDestroyWindow(window_);
	close_ogl();
	return EXIT_SUCCESS;
}


int GLViewer::launch3d()
{
	GLint total_gpu_mem = 0;
	glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &total_gpu_mem);
	if (total_gpu_mem != 0)
	{
		avail_gpu_mem_ = 0;
		glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &avail_gpu_mem_);
		mem_ = double(avail_gpu_mem_) / double(total_gpu_mem);
		std::cout << "Total memory " << total_gpu_mem / 1024 << "MB" << std::endl;
		std::cout << "Available memory " << mem_ << ": " << avail_gpu_mem_ / 1024 << " MB& " << avail_gpu_mem_ % 1024 << " KB" << std::endl;
	}
	locked_ = false;
	init_ogl();


	glfwGetFramebufferSize(window_, &vp_w_, &vp_h_);
	need_redraw_ = true;
	cam_.set_aspect_ratio(double(vp_w_)/vp_h_);
	resize_ogl(vp_w_,vp_h_);
	glViewport(0,0,vp_w_,vp_h_);

	int32_t frame_counter = 0;
	while (!glfwWindowShouldClose(window_))
	{
		if (++frame_counter == 50)
		{
			double now = glfwGetTime();
			frame_counter = 0;
			fps_ = 50 / (now - time_last_50_frames_);
			time_last_50_frames_ = now;

			if (total_gpu_mem != 0)
			{
				avail_gpu_mem_ = 0;
				glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &avail_gpu_mem_);
				mem_ = double(avail_gpu_mem_) / double(total_gpu_mem);
			}
		}	

		glfwPollEvents();
		glfwMakeContextCurrent(window_);
		this->spin();
		this->draw_ogl();
		if (show_imgui_)
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			this->interface_ogl();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
		glfwSwapBuffers(window_);
	}
	glfwDestroyWindow(window_);
	close_ogl();
	return EXIT_SUCCESS;
}


void GLViewer::interface_ogl()
{
	ImGui::GetIO().FontGlobalScale = 2.0f;
	ImGui::Begin("EasyCppOGL Example 1",nullptr, ImGuiWindowFlags_NoSavedSettings);
	ImGui::SetWindowSize({0,0});
	ImGui::Text("FPS :(%2.2lf)", fps_);
	ImGui::End();
}

float current_time()
{
	return float(glfwGetTime());
}


}
