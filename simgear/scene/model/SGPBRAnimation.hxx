/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Fernando García Liñán
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "animation.hxx"

class SGPBRAnimation : public SGAnimation {
public:
    SGPBRAnimation(simgear::SGTransientModelData& modelData);
    virtual ~SGPBRAnimation();
    vsg::Group* createAnimationGroup(vsg::Group& parent) override;
private:
    osgDB::FilePathList _texture_path_list;
};
