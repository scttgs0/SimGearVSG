// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2025 James Turner <james@flightgear.org>

/**
 * @file
 * @brief Allow a canvas::Image element to be used in layouts
 */

#pragma once

#include "simgear/structure/SGSharedPtr.hxx"
#include <simgear/canvas/layout/LayoutItem.hxx>

namespace simgear::canvas {

class Image;

class ImageLayoutItem : public LayoutItem
{
public:
    ImageLayoutItem(SGSharedPtr<Image> img);

    void setGeometry(const SGRecti& geom) override;

    void setResizeCanvas(bool b);

// copied from NasalWidget
    void setSizeHint(const SGVec2i& s);
    void setMinimumSize(const SGVec2i& s);
    void setMaximumSize(const SGVec2i& s);

protected:
    SGVec2i sizeHintImpl() const override;
    SGVec2i minimumSizeImpl() const override;
    SGVec2i maximumSizeImpl() const override;

private:
    SGVec2i _user_size_hint,
            _user_min_size,
            _user_max_size;

    bool _resizeCanvas = false;
    SGSharedPtr<Image> _image;
};

using ImageLayoutItemRef = SGSharedPtr<ImageLayoutItem> ;

} // namespace simgear::canvas
