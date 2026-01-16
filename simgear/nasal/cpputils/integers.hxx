// SPDX-FileCopyrightText: 2025 Florent Rougon
// SPDX-License-Identifier: LGPL-2.0-or-later

/**
 * @file
 * @brief C++ utilities for working with Nasal objects
 */

#pragma once

#include <cmath>
#include <optional>

#include <simgear/nasal/nasal.h>

namespace nasal
{

/**
 * @brief Give access to the integer inside a @c naRef, if any
 *
 * @tparam T    underlying integer type for the return value
 * @param ref   a @c naRef which may or may not contain an integer
 * @return The integer wrapped as an `std::optional<T>`, if any
 *
 * If @a ref is an integer, return this integer as an `std::optional<T>` (the
 * @c double which is really the numeric part of @a ref is cast to @c T and
 * the result is wrapped within an `std::optional<T>`).
 *
 * If @a ref is not an integer, return an `std::optional<T>` that has no
 * value (this includes the case where @a ref is a string containing an
 * integer).
 *
 * Since Nasal numbers are implemented using C's `double` type, the “is
 * integer test” is done with:
 * @code
 * naIsNum(ref) && ref.num == std::floor(ref.num)
 * @endcode
 */
template <class T>
std::optional<T> as_integer(naRef ref)
{
    if (naIsNum(ref) && ref.num == std::floor(ref.num)) {
        return static_cast<T>(ref.num); // wrapped within an std::optional<T>
    }

    return {};
}

} // namespace nasal
