/*
 * SPDX-FileName: galaxy.hxx
 * SPDX-FileComment: model the celestial sphere brightness by unresolved sources
 * SPDX-FileContributor: Chris Ringeval. Started November 2021.
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <vsg/all.h>

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>


namespace osg {
class Node;
}

namespace simgear {
class SGReaderWriterOptions;
}

class SGGalaxy : public SGReferenced
{
public:
    SGGalaxy() = default;

    // build the galaxy object
    vsg::Node* build(double galaxy_size, const simgear::SGReaderWriterOptions* options);
};
