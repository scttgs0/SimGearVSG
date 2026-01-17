
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2025 James Turner <james@flightgear.org>

/**
 * @file
 * @brief Element providing an image (possible a canvas) a layout.
 */

#include "ImageLayoutItem.hxx"
#include "simgear/math/SGRect.hxx"
#include <simgear_config.h>

#include <simgear/canvas/elements/CanvasImage.hxx>


namespace simgear::canvas {

ImageLayoutItem::ImageLayoutItem(SGSharedPtr<Image> img) : _user_size_hint(0, 0),
                                                           _user_min_size(0, 0),
                                                           _user_max_size(MAX_SIZE),
                                                           _image(img)
{
}

void ImageLayoutItem::setGeometry(const SGRecti& geom)
{
    auto srcCanvas = _image->getSrcCanvas().lock();
    const auto w= geom.width();
    const auto h = geom.height();

    if (srcCanvas) {
        if (_resizeCanvas) {
            // match scaling im PUICompatObject::createChildCanvas
            const float aaScale = 2.0;
            srcCanvas->setSizeX(w * aaScale);
            srcCanvas->setSizeY(h * aaScale);
        }
    }
    _image->setTranslation(0, geom.x(), geom.y());
    _image->setSize({static_cast<float>(w), static_cast<float>(h)});
}

SGVec2i ImageLayoutItem::sizeHintImpl() const
{
    SGVec2i hint;
    // default to size of the source canvas / image
    auto srcCanvas = _image->getSrcCanvas().lock();
    if (srcCanvas) {
        hint = {srcCanvas->getViewWidth(), srcCanvas->getViewHeight()};
    } else {
        auto osgImg = _image->getImage();
        hint = {osgImg->s(), osgImg->t()};
    }

// user-specified value overrides
    if (_user_size_hint.x() > 0) {
        hint.x() = _user_size_hint.x();
    }

    if (_user_size_hint.y() > 0) {
        hint.y() = _user_size_hint.y();
    }
    return hint;
}

  //----------------------------------------------------------------------------
  SGVec2i ImageLayoutItem::minimumSizeImpl() const
  {
    return SGVec2i(
      _user_min_size.x() > 0 ? _user_min_size.x() : 16,
      _user_min_size.y() > 0 ? _user_min_size.y() : 16
    );
  }

  //----------------------------------------------------------------------------
  SGVec2i ImageLayoutItem::maximumSizeImpl() const
  {
    return _user_max_size;
  }

void ImageLayoutItem::setResizeCanvas(bool b)
{
    _resizeCanvas = b;
}

  //----------------------------------------------------------------------------
  void ImageLayoutItem::setSizeHint(const SGVec2i& s)
  {
    if( _user_size_hint == s )
      return;

    _user_size_hint = s;

    // TODO just invalidate size_hint? Probably not a performance issue...
    invalidate();
  }

  //----------------------------------------------------------------------------
  void ImageLayoutItem::setMinimumSize(const SGVec2i& s)
  {
    if( _user_min_size == s )
      return;

    _user_min_size = s;
    invalidate();
  }

  //----------------------------------------------------------------------------
  void ImageLayoutItem::setMaximumSize(const SGVec2i& s)
  {
    if( _user_max_size == s )
      return;

    _user_max_size = s;
    invalidate();
  }


} // namespace simgear::canvas
