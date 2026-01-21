// VPBLineFeatureRenderer.hxx -- Renderer for line features
//
// Copyright (C) 2024 Stuart Buchanan
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#pragma once

#include <mutex>

#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/Geometry>

#include <osgTerrain/TerrainTechnique>
#include <osgTerrain/Locator>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/bvh/BVHMaterial.hxx>
#include <simgear/math/SGGeometry.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/tgdb/LightBin.hxx>
#include <simgear/scene/tgdb/LineFeatureBin.hxx>
#include <simgear/scene/tgdb/ObjectInstanceBin.hxx>
#include <simgear/scene/tgdb/VPBBufferData.hxx>

using namespace osgTerrain;

namespace simgear {

class VPBLineFeatureRenderer
{
    public:
        VPBLineFeatureRenderer(vsg::ref_ptr<TerrainTile> tile);

        virtual void applyLineFeatures(BufferData& buffer, vsg::ref_ptr<SGReaderWriterOptions> options, vsg::ref_ptr<SGMaterialCache> matcache);

        static void addLineFeatureList(SGBucket bucket, LineFeatureBinList roadList);
        static void unloadFeatures(SGBucket bucket);

    protected:

        vsg::ref_ptr<osgTerrain::Locator> _masterLocator;
        unsigned int _tileLevel;

        virtual void generateLineFeature(BufferData& buffer, 
            LineFeatureBin::LineFeature road, 
            vsg::mat4 localToWorldMatrix,
            vsg::vec3Array* v, 
            osg::Vec2Array* t, 
            vsg::vec3Array* n,
            std::vector<vsg::vec3>* lights,
            std::vector<float>* rotations,
            SGMaterial* mat);

        virtual vsg::dvec3 getMeshIntersection(BufferData& buffer, vsg::dvec3 pt);

        typedef std::pair<SGBucket, LineFeatureBinList> BucketLineFeatureBinList;

        inline static std::list<BucketLineFeatureBinList>  _lineFeatureLists;
        inline static std::mutex _lineFeatureLists_mutex;  // protects the _lineFeatureLists;
};

};
