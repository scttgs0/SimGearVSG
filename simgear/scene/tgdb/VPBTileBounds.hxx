// VPBTileBounds.hxx -- VirtualPlanetBuilder Tile bounds for clipping
//
// Copyright (C) 2021 Stuart Buchanan
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

#include <osg/Geometry>
#include <osgTerrain/TerrainTile>

using namespace osgTerrain;


class TileBounds {
    public:
        TileBounds(Locator *locator, vsg::dvec3 up);
        virtual std::list<vsg::dvec3> clipToTile(std::list<vsg::dvec3> points);
        virtual bool insideTile(vsg::dvec3 pt);
    
    protected:
        virtual vsg::dvec3 getTileIntersection(vsg::dvec3 inside, vsg::dvec3 outside);
        virtual vsg::dvec3 getPlaneIntersection(vsg::dvec3 inside, vsg::dvec3 outside, vsg::dvec3 normal, vsg::dvec3 plane);

        // Corners of the tile
        vsg::dvec3 v00, v01, v10, v11;

        // Plane normals for the tile bounds
        vsg::dvec3 north, east, south, west;
};
