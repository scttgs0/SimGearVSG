// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Manage event handling inside a Canvas similar to the DOM Level 3 Event Model
 */

#pragma once

#include <deque>

#include <vsg/all.h>

#include "canvas_fwd.hxx"


namespace simgear::canvas {

struct EventTarget {
    ElementWeakPtr element;

    // Used as storage by EventManager during event propagation
    mutable vsg::vec2 local_pos;

    EventTarget(Element* el,
                const vsg::vec2 pos = vsg::vec2()) : element(el),
                                                     local_pos(pos)
    {
    }
};

inline bool operator==(const EventTarget& t1, const EventTarget& t2)
{
    return t1.element.lock() == t2.element.lock();
}

class EventManager
{
public:
    EventManager();

    bool handleEvent(const MouseEventPtr& event,
                     const EventPropagationPath& path);

    bool propagateEvent(const EventPtr& event,
                        const EventPropagationPath& path);

protected:
    struct StampedPropagationPath {
        StampedPropagationPath();
        StampedPropagationPath(const EventPropagationPath& path, double time);

        void clear();
        bool valid() const;

        EventPropagationPath path;
        double time;
    };

    // TODO if we really need the paths modify to not copy around the paths
    //      that much.
    StampedPropagationPath _last_mouse_over;
    size_t _current_click_count;

    struct MouseEventInfo : public StampedPropagationPath {
        int button;
        vsg::vec2 pos;

        void set(const MouseEventPtr& event,
                 const EventPropagationPath& path);
    } _last_mouse_down,
        _last_click;

    /**
       * Propagate click event and handle multi-click (eg. create dblclick)
       */
    bool handleClick(const MouseEventPtr& event,
                     const EventPropagationPath& path);

    /**
       * Handle mouseover/enter/out/leave
       */
    bool handleMove(const MouseEventPtr& event,
                    const EventPropagationPath& path);

    /**
       * Check if two click events (either mousedown/up or two consecutive
       * clicks) are inside a maximum distance to still create a click or
       * dblclick event respectively.
       */
    bool checkClickDistance(const vsg::vec2& pos1,
                            const vsg::vec2& pos2) const;
    EventPropagationPath
    getCommonAncestor(const EventPropagationPath& path1,
                      const EventPropagationPath& path2) const;
};

} // namespace simgear::canvas
