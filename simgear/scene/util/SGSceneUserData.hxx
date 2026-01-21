/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Mathias Froehlich 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#pragma once

#include <vector>

#include <vsg/all.h>

#include <simgear/bvh/BVHNode.hxx>
#include <simgear/structure/SGSourceLocation.hxx>

#include "SGPickCallback.hxx"


class SGSceneUserData : public vsg::Object
{
public:
    //!!META_Object(simgear, SGSceneUserData);
    SGSceneUserData() {}
    SGSceneUserData(const SGSceneUserData& rhs,
                    const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY)
        : vsg::Object(rhs, copyOp),
          _bvhNode(rhs._bvhNode), _velocity(rhs._velocity),
          _pickCallbacks(rhs._pickCallbacks),
          _location(rhs._location)
    {
    }
    static SGSceneUserData* getSceneUserData(vsg::Node* node);
    static const SGSceneUserData* getSceneUserData(const vsg::Node* node);
    static SGSceneUserData* getOrCreateSceneUserData(vsg::Node* node);

    /// Access to the pick callbacks of a node.
    unsigned getNumPickCallbacks() const;
    SGPickCallback* getPickCallback(unsigned i) const;
    void setPickCallback(SGPickCallback* pickCallback);
    void addPickCallback(SGPickCallback* pickCallback);

    const simgear::BVHNode* getBVHNode() const
    {
        return _bvhNode;
    }
    simgear::BVHNode* getBVHNode()
    {
        return _bvhNode;
    }
    void setBVHNode(simgear::BVHNode* bvhNode)
    {
        _bvhNode = bvhNode;
    }

    struct Velocity : public SGReferenced {
        Velocity() : linear(SGVec3d::zeros()),
                     angular(SGVec3d::zeros()),
                     referenceTime(0),
                     id(simgear::BVHNode::getNewId())
        {
        }
        SGVec3d linear;
        SGVec3d angular;
        double referenceTime;
        simgear::BVHNode::Id id;
    };
    const Velocity* getVelocity() const
    {
        return _velocity;
    }
    Velocity* getOrCreateVelocity()
    {
        if (!_velocity) _velocity = new Velocity;
        return _velocity;
    }
    void setVelocity(Velocity* velocity)
    {
        _velocity = velocity;
    }

    const SGSourceLocation& getLocation() const
    {
        return _location;
    }
    void setLocation(const SGSourceLocation& location)
    {
        _location = location;
    }

private:
    // If this node has a collision tree attached, it is stored here
    SGSharedPtr<simgear::BVHNode> _bvhNode;

    // Velocity in the childs local coordinate system
    SGSharedPtr<Velocity> _velocity;

    /// Scene interaction callbacks
    std::vector<SGSharedPtr<SGPickCallback>> _pickCallbacks;

    /// Original source location describing this node
    SGSourceLocation _location;
};
