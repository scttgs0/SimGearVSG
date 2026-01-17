// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#pragma once

#include <simgear/ephemeris/celestialBody.hxx>
#include <simgear/ephemeris/star.hxx>

class Uranus : public CelestialBody
{
public:
  Uranus (double mjd);
  Uranus ();
  void updatePosition(double mjd, Star *ourSun);
};
