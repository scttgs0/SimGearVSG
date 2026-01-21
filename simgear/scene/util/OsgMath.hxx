// Copyright (C) 2006-2009  Mathias Froehlich - Mathias.Froehlich@web.de
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

#include <osg/Quat>

#include <simgear/math/SGMath.hxx>


inline SGVec2d
toSG(const vsg::dvec2& v)
{
    return SGVec2d(v[0], v[1]);
}

inline SGVec2f
toSG(const vsg::vec2& v)
{
    return SGVec2f(v[0], v[1]);
}

inline vsg::dvec2
toOsg(const SGVec2d& v)
{
    return vsg::dvec2(v[0], v[1]);
}

inline vsg::vec2
toOsg(const SGVec2f& v)
{
    return vsg::vec2(v[0], v[1]);
}

inline SGVec3d
toSG(const vsg::dvec3& v)
{
    return SGVec3d(v[0], v[1], v[2]);
}

inline SGVec3f
toSG(const vsg::vec3& v)
{
    return SGVec3f(v[0], v[1], v[2]);
}

inline vsg::dvec3
toOsg(const SGVec3d& v)
{
    return vsg::dvec3(v[0], v[1], v[2]);
}

inline vsg::vec3
toOsg(const SGVec3f& v)
{
    return vsg::vec3(v[0], v[1], v[2]);
}

inline SGVec4d
toSG(const vsg::dvec4& v)
{
    return SGVec4d(v[0], v[1], v[2], v[3]);
}

inline SGVec4f
toSG(const vsg::vec4& v)
{
    return SGVec4f(v[0], v[1], v[2], v[3]);
}

inline vsg::dvec4
toOsg(const SGVec4d& v)
{
    return vsg::dvec4(v[0], v[1], v[2], v[3]);
}

inline vsg::vec4
toOsg(const SGVec4f& v)
{
    return vsg::vec4(v[0], v[1], v[2], v[3]);
}

inline SGQuatd
toSG(const osg::Quat& q)
{
    return SGQuatd(q[0], q[1], q[2], q[3]);
}

inline osg::Quat
toOsg(const SGQuatd& q)
{
    return osg::Quat(q[0], q[1], q[2], q[3]);
}

// Create a local coordinate frame in the earth-centered frame of
// reference. X points north, Z points down.
// makeSimulationFrameRelative() only includes rotation.
inline vsg::mat4
makeSimulationFrameRelative(const SGGeod& geod)
{
    return vsg::mat4(toOsg(SGQuatd::fromLonLat(geod)));
}

inline vsg::mat4
makeSimulationFrame(const SGGeod& geod)
{
    vsg::mat4 result(makeSimulationFrameRelative(geod));
    SGVec3d coord;
    SGGeodesy::SGGeodToCart(geod, coord);
    result.setTrans(toOsg(coord));
    return result;
}

// Create a Z-up local coordinate frame in the earth-centered frame
// of reference. This is what scenery models, etc. expect.
// makeZUpFrameRelative() only includes rotation.
inline vsg::mat4
makeZUpFrameRelative(const SGGeod& geod)
{
    vsg::mat4 result(makeSimulationFrameRelative(geod));
    // 180 degree rotation around Y axis
    result.preMultRotate(osg::Quat(0.0, 1.0, 0.0, 0.0));
    return result;
}

inline vsg::mat4
makeZUpFrame(const SGGeod& geod)
{
    vsg::mat4 result(makeZUpFrameRelative(geod));
    SGVec3d coord;
    SGGeodesy::SGGeodToCart(geod, coord);
    result.setTrans(toOsg(coord));
    return result;
}
