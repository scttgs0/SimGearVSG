// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023 Stuart Buchanan <stuart13@gmail.com>

#pragma once

#include "easyxml.hxx"

#include <vsg/all.h>

#include <osgTerrain/TerrainTile>

#include <simgear/structure/SGSharedPtr.hxx>

#include "BVHGroup.hxx"
#include "BVHLineSegmentVisitor.hxx"
#include "BVHMaterial.hxx"
#include "BVHVisitor.hxx"


namespace simgear {

class BVHTerrainTile : public BVHGroup
{
public:
    BVHTerrainTile(osgTerrain::TerrainTile* tile);
    virtual ~BVHTerrainTile();
    virtual void accept(BVHVisitor& visitor);
    virtual SGSphered computeBoundingSphere() const;
    BVHMaterial* getMaterial(BVHLineSegmentVisitor* lsv);

private:
    vsg::ref_ptr<osgTerrain::TerrainTile> _tile;
};

} // namespace simgear
