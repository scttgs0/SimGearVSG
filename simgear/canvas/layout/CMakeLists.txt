# SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

include (SimGearComponent)

set(HEADERS
  AlignFlag_values.hxx
  BoxLayout.hxx
  GridLayout.hxx
  ImageLayoutItem.hxx
  Layout.hxx
  LayoutItem.hxx
  NasalWidget.hxx
  SpacerItem.hxx
)

set(SOURCES
  BoxLayout.cxx
  GridLayout.cxx
  ImageLayoutItem.cxx
  Layout.cxx
  LayoutItem.cxx
  NasalWidget.cxx
  SpacerItem.cxx
)

simgear_scene_component(canvas-layout canvas/layout "${SOURCES}" "${HEADERS}")

add_boost_test(canvas_layout
  SOURCES canvas_layout_test.cxx
  LIBRARIES SimGearScene
)
