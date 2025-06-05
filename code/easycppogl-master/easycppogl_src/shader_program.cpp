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

/*******************************************************************************
* CGoGN: Combinatorial and Geometric modeling with Generic N-dimensional Maps  *
* Copyright (C) 2015, IGG Group, ICube, University of Strasbourg, France       *
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
* Web site: http://cgogn.unistra.fr/                                           *
* Contact information: cgogn@unistra.fr                                        *
*                                                                              *
*******************************************************************************/

#include <shader_program.h>
#include<thread>
#include <fstream>
#include<streambuf>
#include <iterator>
#include <string>
#include <utility>
#include <iomanip>
#include <ubo.h>

#pragma warning( disable : 4244 4018)

bool Uniform_Explicit_Location_Support=true;


namespace EZCOGL
{

GLuint UBO::binding_points_=0;

std::string load_src(const std::string& full_path_name)
{
    auto ifs = std::ifstream(full_path_name);
    return std::string(std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>());
}

//std::string load(const std::string& file)
//{
//	std::ifstream fs(file);
//	if (!fs.good())
//	{
//		std::cerr << "Could not open shader file " << file << std::endl;
//		return "";
//	}
//	std::stringstream sst;
//	sst << fs.rdbuf();
//	fs.close();
//	return sst.str();
//}

const ShaderProgram* ShaderProgram::current_binded_ = nullptr;

void Shader::compile(const std::string& src, const std::string& name)
{	
#ifdef __APPLE__
	std::string src_cp(src);
	auto ind = src_cp.find("#version");
	if (ind	!= std::string::npos)
	{
		auto ind2 = src_cp.find("430",ind);
		if ((ind2 != std::string::npos) && (ind2-ind <12))
		{
			src_cp[ind2]='3';
		}
	}
	const char* csrc = src_cp.c_str();
#else
	const char* csrc = src.c_str();
#endif

	glShaderSource(id_, 1, &csrc, nullptr);
	glCompileShader(id_);

	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

	glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &infologLength);

	if(infologLength > 1)
	{
		infoLog =  new char[infologLength];
		glGetShaderInfoLog(id_, infologLength, &charsWritten, infoLog);

		std::cerr << "----------------------------------------" << std::endl << "compilation de " << name <<  " : "<<std::endl << infoLog <<std::endl<< "--------"<< std::endl;

		std::string errors(infoLog);
		std::istringstream sserr(errors);
		std::vector<int> error_lines;
		std::string line;
		std::getline(sserr, line);
		while (! sserr.eof())
		{
			std::size_t a =0;
			while ((a<line.size()) && (line[a]!='(')) a++;
			std::size_t b =a+1;
			while ((b<line.size()) && (line[b]!=')')) b++;
			if (b<line.size())
			{
				int ln = std::stoi(line.substr(a+1, b-a-1));
				error_lines.push_back(ln);
			}
			std::getline(sserr, line);
		}

		free(infoLog);

		char* source = new char[16*1024];
		GLsizei length;
		glGetShaderSource(id_,16*1024,&length,source);
		std::string sh_src(source);
		std::istringstream sssrc(sh_src);

#ifdef WIN32
		HANDLE  hConsole;
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, 12);
		int l = 1;
		while (! sssrc.eof())
		{
			std::getline(sssrc, line);
			auto it = std::find(error_lines.begin(),error_lines.end(),l);
			if (it != error_lines.end())
				SetConsoleTextAttribute(hConsole, 16*12+15);
			else
				SetConsoleTextAttribute(hConsole, 15);
			std::cout.width(3);
			std::cout<< l << " : " << line << std::endl;
			l++;
		}
		SetConsoleTextAttribute(hConsole, 15);
#else
		int l = 1;
		while (! sssrc.eof())
		{
			std::getline(sssrc, line);
			auto it = std::find(error_lines.begin(),error_lines.end(),l);
			if (it != error_lines.end())
				std::cout << "\033[41m\033[37m";
			else
				std::cout<< "\033[m";
			std::cout.width(3);
			std::cout<< l << " : " << line << std::endl;
			l++;
		}
		std::cout << "----------------------------------------" << std::endl;
#endif
	}
}

std::string string_type_of_shader(GLenum ToS)
{
	switch (ToS)
	{
		case GL_VERTEX_SHADER: return "VERTEX_SHADER";// break;
		case GL_FRAGMENT_SHADER: return "FRAGMENT_SHADER";// break;
		case GL_GEOMETRY_SHADER: return "GEOMETRY_SHADER";// break;
		case GL_TESS_CONTROL_SHADER: return "TESS_CONTROL_SHADER"; //break;
		case GL_TESS_EVALUATION_SHADER : return "TESS_EVALUATION_SHADER";// break;
		case GL_COMPUTE_SHADER: return "COMPUTE_SHADER"; //break;
		default: return "???"; //break;
	}
}

void ShaderProgram::location_analyser(std::string& src)
{
	std::istringstream ss(src);
	char buff[256];
	char* buf_ptr = buff;

	while (!ss.eof())
	{
		int begin_of_line = ss.tellg();
		ss.getline(buff,256,'\n');
		std::string line(buff);

		std::size_t pos1 = line.find("layout",0);
		if (pos1 != std::string::npos)
		{
			std::size_t pos2 = line.find("location",pos1);
			if (pos2 != std::string::npos)
			{
				std::size_t pos3 = line.find("=",0);
				if (pos3 != std::string::npos)
				{
					buf_ptr = buff+pos3+1;
					std::string lstr(buf_ptr);
					std::istringstream lss(lstr);
					int loc_val;
					lss >> loc_val;
					std::size_t pos4 = line.find("uniform ",0);
					if (pos4 != std::string::npos)
					{
						buf_ptr = buff+pos4+8;
						std::string lstr(buf_ptr);
						std::istringstream lss(lstr);
						std::string tmp_name;
						lss >> tmp_name; // type
						lss >> tmp_name;
						std::size_t pos_end = tmp_name.find_last_of(';');
						std::string loc_name = (pos_end != std::string::npos) ? tmp_name.substr(0,pos_end) : tmp_name;
						this->ulocations[loc_name] = loc_val;
						std::cout << "Location of "<<loc_name<<" is "<<loc_val<<std::endl;
						for (int i =0; i <pos4; ++i)
							src[begin_of_line+i] = ' ';
					}
				}
			}
		}
	}

}

//void ShaderProgram::init(const std::vector<std::pair<GLenum,const std::string&>>& sources,  const std::string& name, const std::vector<std::string>& tf_outs) 
ShaderProgram::ShaderProgram(const std::vector<std::pair<GLenum, const std::string&>>& sources, const std::string& name, const std::vector<std::string>& tf_outs):
	name_(name)
{
	/*name_ = name;*/
	utranslat.resize(256);
	for (int i=0;i<256;++i)
		utranslat[i] = i;

	id_ = glCreateProgram();
////V1

	if (Uniform_Explicit_Location_Support)
	{
		for (const auto& sh: sources)
			{
				Shader* shader = new Shader(sh.first);
				shader->compile(sh.second, name_+"::"+string_type_of_shader(sh.first));
				shaders_.push_back(shader);
				glAttachShader(id_, shader->shaderId());
			}
	}
	else
	{
		for (const auto& sh: sources)
		{
			std::string src = sh.second;
			location_analyser(src);
			Shader* shader = new Shader(sh.first);
			shader->compile(src, name_+"::"+string_type_of_shader(sh.first));
			shaders_.push_back(shader);
			glAttachShader(id_, shader->shaderId());
		}
	}

	if (!tf_outs.empty())
	{
		std::vector<const char*> tfo;
		for (const auto& t: tf_outs)
			tfo.push_back(t.c_str());
		glTransformFeedbackVaryings(id_, GLsizei(tf_outs.size()), tfo.data(), GL_SEPARATE_ATTRIBS);
	}

	glLinkProgram(id_);

	for (auto sh: shaders_)
	{
		glDetachShader(id_, sh->shaderId());
	}

	if (!Uniform_Explicit_Location_Support)
	{
		for (const auto& p: ulocations)
		{
			auto uni = glGetUniformLocation(this->id_,p.first.c_str());
			this->utranslat[p.second] = uni;
			std::cout << "Uniform "<<p.first<< " GL: "<< uni<< " User: "<<p.second<<std::endl;
		}
	}
	int infologLength = 0;
	glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 1)
	{
		char* infoLog = new char[infologLength];
		int charsWritten = 0;
		glGetProgramInfoLog(id_, infologLength, &charsWritten, infoLog);
		std::cerr << "Link message :" << name << " :" << std::endl<< infoLog << std::endl;
		delete[] infoLog;
	}
}




ShaderProgram::UP ShaderProgram::create(const std::vector<std::pair<GLenum,const std::string&>>& sources,  const std::string& name,const std::vector<std::string>& tf_outs)
{
	auto unp = std::unique_ptr<ShaderProgram>(new ShaderProgram(sources,name,tf_outs));
	GLint status;
	glGetProgramiv(unp->id(),GL_LINK_STATUS,&status);
	if (status == GL_FALSE)
	{
		std::cerr << "Error in compiling ShaderProgram "<<name<<std::endl;
		return nullptr;
	}
	return unp;
}



ShaderProgram::~ShaderProgram()
{
	for (auto ptr : shaders_)
		delete ptr;
	glDeleteProgram(id_);
}

} // namespace
