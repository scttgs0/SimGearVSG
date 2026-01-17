// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Element providing blank space in a layout.
 */

#pragma once

#include "LayoutItem.hxx"

namespace simgear::canvas
{
  /**
   * Element for providing blank space in a layout.
   */
  class SpacerItem:
    public LayoutItem
  {
    public:
      SpacerItem( const SGVec2i& size = SGVec2i(0, 0),
                  const SGVec2i& max_size = MAX_SIZE );

  };

using SpacerItemRef = SGSharedPtr<SpacerItem> ;

} // namespace simgear::canvas
