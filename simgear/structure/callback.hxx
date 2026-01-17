// SPDX-License-Identifier: LGPL-2.1-or-later

/**
 * @file
 * @brief Declaration for simgear callback
*/

#pragma once

#include <functional>

namespace simgear {
    using Callback = std::function<void()>;
}
