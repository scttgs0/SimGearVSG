// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1998 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Routines to handle linear interpolation from a table of x,y. The table must be sorted by "x" in ascending order
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <string>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/exception.hxx>

#include "interpolater.hxx"

// Constructor -- starts with an empty table.
SGInterpTable::SGInterpTable()
{
}

SGInterpTable::SGInterpTable(const SGPropertyNode* interpolation)
{
    if (!interpolation)
        throw sg_exception("Missing table config!");

    std::vector<SGPropertyNode_ptr> entries = interpolation->getChildren("entry");
    if (entries.size() == 0)
        throw sg_exception("Table config has no entries!");

    for (unsigned i = 0; i < entries.size(); ++i)
        addEntry(entries[i]->getDoubleValue("ind", 0.0),
                 entries[i]->getDoubleValue("dep", 0.0));
}

// Constructor -- loads the interpolation table from the specified
// file
SGInterpTable::SGInterpTable( const std::string& file )
{
    sg_gzifstream in( SGPath::fromUtf8(file) );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
        return;
    }

    in >> skipcomment;
    while ( in ) {
        double ind, dep;
        in >> ind >> dep;
        in >> std::skipws;
        _table[ind] = dep;
    }
}


// Constructor -- loads the interpolation table from the specified
// file
SGInterpTable::SGInterpTable( const SGPath& file )
{
    sg_gzifstream in( file );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
        return;
    }

    in >> skipcomment;
    while ( in ) {
      double ind, dep;
      in >> ind >> dep;
      in >> std::skipws;
      _table[ind] = dep;
    }
}


// Add an entry to the table.
void SGInterpTable::addEntry (double ind, double dep)
{
  _table[ind] = dep;
}

// find lower and upper bound for a given x
// if x is outside the table range, first or last table element will be used
std::optional<SGInterpTable::Bounds>
SGInterpTable::findBounds(double x) const
{
    if (_table.empty()) {
        return std::nullopt;
    }

    const auto up = _table.upper_bound(x);

    // x <= smallest table index
    if (up == _table.begin()) {
        return Bounds{up, up};
    }

    // table not empty and up not first element
    auto lo = std::prev(up);

    // x > largest table index, return last element
    if (up == _table.end()) {
        return Bounds{lo, lo};
    }

    return Bounds{lo, up};
}


// Given an x value, linearly interpolate the y value from the table
double SGInterpTable::interpolate(double x) const
{
  auto bounds = findBounds(x);
  if (!bounds) {
    throw sg_exception("SGInterpTable::interpolate() empty table.");
  }

  auto [loBoundIt, upBoundIt] = *bounds;
  if (loBoundIt == upBoundIt) {
    return loBoundIt->second;
  }

  // Just do linear interpolation.
  double loBound = loBoundIt->first;
  double upBound = upBoundIt->first;
  double loVal = loBoundIt->second;
  double upVal = upBoundIt->second;

  // division by zero should not happen since the std::map
  // has sorted out duplicate entries before. Also since we have a
  // map, we know that we get different first values for different iterators
  return loVal + (upVal - loVal)*(x - loBound)/(upBound - loBound);
}

// Given an x value, find nearest y value from the table
double SGInterpTable::nearest(double x) const
{
  auto bounds = findBounds(x);
  if (!bounds) {
    throw sg_exception("SGInterpTable::nearest() empty table.");
  }

  auto [lo, up] = *bounds;
  if (lo == up) {
    return lo->second;
  }

  // check if x is closer to lower or upper table index
  return (x - lo->first < up->first - x) ? lo->second : up->second;
}


// Destructor
SGInterpTable::~SGInterpTable() {
}
