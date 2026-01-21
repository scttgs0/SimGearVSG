// SPDX-FileCopyrightText: Copyright (C) 2017 Richard Harrison
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>

#include <vsg/all.h>

#include <osg/NodeVisitor>

namespace simgear {

class FindGroupVisitor : public osg::NodeVisitor
{
public:
    FindGroupVisitor(const std::string& name);

    vsg::Group* getGroup() const
    {
        return _group.get();
    }

    bool foundDuplicates() const
    {
        return _duplicates;
    }

    virtual void apply(vsg::Group& group);

protected:
    std::string _name;
    vsg::ref_ptr<vsg::Group> _group;
    bool _duplicates;
};

} // namespace simgear
