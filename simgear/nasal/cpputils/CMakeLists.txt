# SPDX-FileCopyrightText: 2025 Florent Rougon
# SPDX-License-Identifier: LGPL-2.0-or-later

include (SimGearComponent)

set(HEADERS
    integers.hxx
    )

set(SOURCES
    )

simgear_component(nasal/cpputils nasal/cpputils "${SOURCES}" "${HEADERS}")
