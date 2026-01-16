// apt_signs.hxx -- build airport signs on the fly
//
// Written by Curtis Olson, started July 2001.
//
// SPDX-FileCopyrightText: 2001 Curtis L. Olson - http://www.flightgear.org/~curt
// SPDX-License-Identifier: GPL-2.0-or-later


#ifndef _SG_APT_SIGNS_HXX
#define _SG_APT_SIGNS_HXX

#include <simgear/compiler.h>

#include <string>
#include <memory> // for auto-ptr

#include <osg/Node>

class SGMaterialLib;            // forward declaration
class SGGeod;

// Generate a generic sign
osg::Node* SGMakeSign( SGMaterialLib *matlib, const std::string& content );

namespace simgear
{

class AirportSignBuilder final
{
public:
    AirportSignBuilder(SGMaterialLib* mats, const SGGeod& center);
    ~AirportSignBuilder();      // non-virtual intentional

    void addSign(const SGGeod& pos, double heading, const std::string& content, int size);

    osg::Node* getSignsGroup();
private:
    class AirportSignBuilderPrivate;
    std::unique_ptr<AirportSignBuilderPrivate> d;
};

} // of namespace simgear

#endif // _SG_APT_SIGNS_HXX
