// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Easing functions for property interpolation.
 *
 * Based on easing functions by Robert Penner (http://www.robertpenner.com/easing)
 */

#pragma once

namespace simgear
{

  typedef double (*easing_func_t)(double);
  struct EasingMapEntry { const char* name; easing_func_t func; };

  /**
   * List of all available easing functions and their names.
   */
  extern const EasingMapEntry easing_functions[];

} // namespace simgear
