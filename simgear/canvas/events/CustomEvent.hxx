// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas user defined event
 */

#pragma once

#include <simgear/canvas/CanvasEvent.hxx>
#include <simgear/structure/map.hxx>

namespace simgear::canvas
{

  /**
   * User defined event (optionally carrying additional context information or
   * data).
   */
  class CustomEvent:
    public Event
  {
    public:

      /**
       * @brief Construct a user defined event from a type string
       *
       * @param type_str    Event type name (if name does not exist yet it will
       *                    be registered as new event type)
       * @param bubbles     If this event should take part in the bubbling phase
       * @param data        Optional user data stored in event
       */
      CustomEvent( std::string const& type_str,
                   bool bubbles = false,
                   StringMap const& data = StringMap() );

      /**
       * @brief Construct a user defined event from a (previously registered)
       *        type id
       *
       * @see getOrRegisterType()
       *
       * @param type_id     Event type id
       * @param bubbles     If this event should take part in the bubbling phase
       * @param data        Optional user data stored in event
       */
      CustomEvent( int type_id,
                   bool bubbles = false,
                   StringMap const& data = StringMap() );

      CustomEvent* clone(int type = 0) const override;

      /**
       * Set user data
       */
      void setDetail(StringMap const& data);

      /**
       * Get user data
       */
      StringMap const& getDetail() const { return detail; }

      /**
       * Get whether this event supports bubbling.
       *
       * @see #bubbles
       * @see CustomEvent()
       */
      bool canBubble() const override { return bubbles; }

      StringMap detail; //!< User data map
      bool bubbles;     //!< Whether the event supports bubbling
  };

} // namespace simgear::canvas
