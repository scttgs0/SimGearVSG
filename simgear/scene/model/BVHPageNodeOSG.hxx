/*
 * SPDX-FileName: BVHPageNodeOSG.hxx
 * SPDX-FileComment: Bounding Volume Hierarchy for OSG
 * SPDX-FileCopyrightText: Copyright (C) 2008- 2025  Mathias Froehlich
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <string>

#include "../../bvh/BVHPageNode.hxx"

#include <osg/Referenced>
#include <osg/ref_ptr>

namespace simgear {

class BVHPageNodeOSG : public BVHPageNode
{
public:
    BVHPageNodeOSG(const std::string& name, const SGSphered& boundingSphere,
                   const osg::ref_ptr<const osg::Referenced>& options);
    BVHPageNodeOSG(const std::vector<std::string>& nameList,
                   const SGSphered& boundingSphere,
                   const osg::ref_ptr<const osg::Referenced>& options);
    virtual ~BVHPageNodeOSG();

    virtual BVHPageRequest* newRequest();

    void setBoundingSphere(const SGSphered& sphere);

    static SGSharedPtr<BVHNode> load(const std::string& name, const osg::ref_ptr<const osg::Referenced>& options, bool forceFlatter = false);
    static SGSharedPtr<BVHNode> load(const string_list nameList, const osg::ref_ptr<const osg::Referenced>& options, bool forceFlatter = false);

protected:
    virtual SGSphered computeBoundingSphere() const;
    virtual void invalidateBound();

private:
    class _NodeVisitor;
    class _Request;

    /// The submodels appropriate for intersection tests.
    std::vector<std::string> _modelList;
    /// The bounding sphere as given by the lod node.
    SGSphered _boundingSphere;
    /// The osg loader options that are active for this subtree
    osg::ref_ptr<const osg::Referenced> _options;
};

} // namespace simgear
