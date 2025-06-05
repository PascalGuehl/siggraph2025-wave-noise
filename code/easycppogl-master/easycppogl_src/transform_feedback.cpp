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

#include <transform_feedback.h>
#include <fstream>

namespace EZCOGL
{

TransformFeedback::TransformFeedback(const std::vector<std::pair<GLenum,const std::string&>>& sources, const std::vector<std::string>& outs, const std::string& name)
{
    prg_ = ShaderProgram::create(sources, name, outs);
    glCreateTransformFeedbacks(1,& id_);
}


void TransformFeedback::start(GLenum prim, std::vector<std::shared_ptr<VBO>> vbos)
{
	prg_->bind();
	glEnable(GL_RASTERIZER_DISCARD);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, id_);
	for (GLuint i = 0; i < vbos.size(); ++i)
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, vbos[i]->id());
    glBeginTransformFeedback(prim);
}

void TransformFeedback::stop()
{
	ShaderProgram::unbind();
    glEndTransformFeedback();
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK,0);
	glDisable(GL_RASTERIZER_DISCARD);
}


} // namespace
