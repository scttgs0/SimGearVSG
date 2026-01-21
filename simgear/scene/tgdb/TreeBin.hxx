/* -*-c++-*-
 *
 * Copyright (C) 2008 Stuart Buchanan
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

#include <string>
#include <vector>

#include <vsg/all.h>

#include <osg/Geometry>
#include <osg/LOD>

#include <simgear/scene/util/OsgMath.hxx>


namespace simgear {
class TreeBin final
{
public:
    TreeBin() = default;
    TreeBin(const SGMaterial* mat);
    TreeBin(const SGPath& absoluteFileName, const SGMaterial* mat);

    ~TreeBin() = default; // non-virtual intentional

    int texture_varieties;
    double range;
    float height;
    float width;
    std::string texture;
    std::string normal_map;
    std::string teffect;

    void insert(vsg::dvec3 t)
    {
        _trees.push_back(t);
    }

    void insert(const SGVec3f& p)
    {
        _trees.push_back(toOsg(p));
    }

    unsigned getNumTrees() const
    {
        return _trees.size();
    }

    const vsg::dvec3 getTree(unsigned i) const
    {
        assert(i < _trees.size());
        return _trees.at(i);
    }

    std::vector<vsg::dvec3> _trees;
};

typedef std::list<TreeBin*> SGTreeBinList;

vsg::Group* createForest(SGTreeBinList& forestList, vsg::ref_ptr<simgear::SGReaderWriterOptions> options);
} // namespace simgear
