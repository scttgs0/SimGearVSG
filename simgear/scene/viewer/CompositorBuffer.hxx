// Copyright (C) 2018 - 2023 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <vsg/all.h>

#include <osg/Texture>


class SGPropertyNode;

namespace simgear {

class SGReaderWriterOptions;

namespace compositor {

class Compositor;

struct Buffer : public osg::Referenced {
    vsg::ref_ptr<osg::Texture> texture;

    /**
     * The amount to multiply the size of the default framebuffer.
     * A factor of 0.0 means that the buffer has a fixed size.
     */
    float width_scale = 0.0f;
    float height_scale = 0.0f;

    /// Whether this is an MVR buffer.
    bool mvr = false;
};

Buffer* buildBuffer(Compositor* compositor, const SGPropertyNode* node,
                    const SGReaderWriterOptions* options);

} // namespace compositor
} // namespace simgear
