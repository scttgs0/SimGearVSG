// Copyright (C) 2008 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#pragma once

#include <vsg/all.h>

#include <osg/PrimitiveSet>


namespace simgear {

class PrimitiveCollector : public osg::PrimitiveFunctor
{
public:
    PrimitiveCollector();
    virtual ~PrimitiveCollector();

    void swap(PrimitiveCollector& primitiveFunctor);

    virtual void setVertexArray(unsigned int count, const vsg::vec2* vertices);
    virtual void setVertexArray(unsigned int count, const vsg::vec3* vertices);
    virtual void setVertexArray(unsigned int count, const vsg::vec4* vertices);
    virtual void setVertexArray(unsigned int count, const vsg::dvec2* vertices);
    virtual void setVertexArray(unsigned int count, const vsg::dvec3* vertices);
    virtual void setVertexArray(unsigned int count, const vsg::dvec4* vertices);

    virtual void drawArrays(GLenum mode, GLint first, GLsizei count);

    template <typename index_type>
    void drawElementsTemplate(GLenum mode, GLsizei count, const index_type* indices);
    virtual void drawElements(GLenum mode, GLsizei count, const GLubyte* indices);
    virtual void drawElements(GLenum mode, GLsizei count, const GLushort* indices);
    virtual void drawElements(GLenum mode, GLsizei count, const GLuint* indices);

    virtual void begin(GLenum mode);
    virtual void vertex(const vsg::vec2& v);
    virtual void vertex(const vsg::vec3& v);
    virtual void vertex(const vsg::vec4& v);
    virtual void vertex(float x, float y);
    virtual void vertex(float x, float y, float z);
    virtual void vertex(float x, float y, float z, float w);
    virtual void end();

    void addVertex(const vsg::dvec3& v);
    void addVertex(const vsg::dvec4& v);

    void addPoint(unsigned i1);
    void addLine(unsigned i1, unsigned i2);
    void addTriangle(unsigned i1, unsigned i2, unsigned i3);
    void addQuad(unsigned i1, unsigned i2, unsigned i3, unsigned i4);

    /// The callback functions that are called on an apropriate primitive
    virtual void addPoint(const vsg::dvec3& v1) = 0;
    virtual void addLine(const vsg::dvec3& v1, const vsg::dvec3& v2) = 0;
    virtual void addTriangle(const vsg::dvec3& v1, const vsg::dvec3& v2, const vsg::dvec3& v3) = 0;

private:
    std::vector<vsg::dvec3> _vertices;
    GLenum _mode;
};

} // namespace simgear
