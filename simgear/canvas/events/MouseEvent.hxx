// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas mouse event
 */

#pragma once

#include <vsg/all.h>

#include "DeviceEvent.hxx"


namespace simgear::canvas {

/**
   * Mouse (button/move/wheel) event
   */
class MouseEvent : public DeviceEvent
{
public:
    MouseEvent();
    MouseEvent(const osgGA::GUIEventAdapter& ea);
    MouseEvent* clone(int type = 0) const override;

    bool canBubble() const override;

    vsg::vec2 getScreenPos() const { return screen_pos; }
    vsg::vec2 getClientPos() const { return client_pos; }
    vsg::vec2 getLocalPos() const { return local_pos; }
    vsg::vec2 getDelta() const { return delta; }

    float getScreenX() const { return screen_pos.x; }
    float getScreenY() const { return screen_pos.y; }

    float getClientX() const { return client_pos.x; }
    float getClientY() const { return client_pos.y; }

    float getLocalX() const { return local_pos.x; }
    float getLocalY() const { return local_pos.y; }

    float getDeltaX() const { return delta.x; }
    float getDeltaY() const { return delta.y; }

    int getButton() const { return button; }
    int getButtonMask() const { return buttons; }

    int getCurrentClickCount() const { return click_count; }

    vsg::vec2 screen_pos, //!< Position in screen coordinates
        client_pos,       //!< Position in window/canvas coordinates
        local_pos,        //!< Position in local/element coordinates
        delta;
    int button,      //!< Button for this event
        buttons,     //!< Current button state
        click_count; //!< Current click count
};

} // namespace simgear::canvas
