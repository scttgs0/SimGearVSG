// placement.cxx - manage the placement of a 3D model.
// Written by David Megginson, started 2002.
//
// SPDX-FileCopyrightText: 2002 David Megginson
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "simgear/debug/debug_types.h"

#include <simgear_config.h>

#include "placement.hxx"

#include <simgear/compiler.h>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>


////////////////////////////////////////////////////////////////////////
// Implementation of SGModelPlacement.
////////////////////////////////////////////////////////////////////////

SGModelPlacement::SGModelPlacement () :
    _position(SGGeod::fromRad(0, 0)),
    _roll_deg(0),
    _pitch_deg(0),
    _heading_deg(0),
    _selector(new osg::Switch),
    _transform(new osg::PositionAttitudeTransform)
{
    _selector->addChild(_transform.get());
}

SGModelPlacement::~SGModelPlacement ()
{
    if (_selector && (_selector->getNumParents() > 0)) {
        SG_LOG(SG_VSG, SG_DEV_WARN, "Destroying model placement with still-attached scene-node.");
    }
}

void
SGModelPlacement::init( vsg::Node * model )
{
  // remove previous models (in case of reinit)
  _transform->removeChild(0, _transform->getNumChildren());
  if (model != 0) {
      _transform->addChild(model);
  }
  _selector->setValue(0, 1);
}

vsg::ref_ptr<vsg::Node> SGModelPlacement::getSceneGraph() const
{
    return _selector;
}


void
SGModelPlacement::add( vsg::Node* model )
{
    if (model != 0) {
        _transform->addChild(model);
    }
}

void SGModelPlacement::clear()
{
    _selector = nullptr;
    _transform = nullptr;
}

void
SGModelPlacement::update()
{
  // The cartesian position
  SGVec3d position = SGVec3d::fromGeod(_position);
  _transform->setPosition(toOsg(position));

  SGQuatd orient = getGlobalOrientation();
  _transform->setAttitude(toOsg(orient));
}

bool
SGModelPlacement::getVisible () const
{
  return _selector->getValue(0);
}

void
SGModelPlacement::setVisible (bool visible)
{
  _selector->setValue(0, visible);
}

void
SGModelPlacement::setPosition(const SGGeod& position)
{
  _position = position;
}

SGQuatd SGModelPlacement::getGlobalOrientation() const
{
    // The orientation, composed from the horizontal local orientation and the
    // orientation wrt the horizontal local frame
    SGQuatd orient = SGQuatd::fromLonLat(_position);
    orient *= SGQuatd::fromYawPitchRollDeg(_heading_deg, _pitch_deg, _roll_deg);
    // Convert to the scenegraph orientation where we just rotate around
    // the y axis 180 degrees.
    orient *= SGQuatd::fromRealImag(0, SGVec3d(0, 1, 0));
    return orient;
}

void
SGModelPlacement::setRollDeg (double roll_deg)
{
  _roll_deg = roll_deg;
}

void
SGModelPlacement::setPitchDeg (double pitch_deg)
{
  _pitch_deg = pitch_deg;
}

void
SGModelPlacement::setHeadingDeg (double heading_deg)
{
  _heading_deg = heading_deg;
}

void
SGModelPlacement::setOrientation (double roll_deg, double pitch_deg,
                                  double heading_deg)
{
  _roll_deg = roll_deg;
  _pitch_deg = pitch_deg;
  _heading_deg = heading_deg;
}

void
SGModelPlacement::setOrientation (const SGQuatd& orientation)
{
  orientation.getEulerDeg(_heading_deg, _pitch_deg, _roll_deg);
}

void
SGModelPlacement::setReferenceTime(const double& referenceTime)
{
  SGSceneUserData* userData;
  userData = SGSceneUserData::getOrCreateSceneUserData(_transform.get());
  SGSceneUserData::Velocity* vel = userData->getOrCreateVelocity();
  vel->referenceTime = referenceTime;
}

void
SGModelPlacement::setBodyLinearVelocity(const SGVec3d& linear)
{
  SGSceneUserData* userData;
  userData = SGSceneUserData::getOrCreateSceneUserData(_transform.get());
  SGSceneUserData::Velocity* vel = userData->getOrCreateVelocity();
  vel->linear = SGVec3d(-linear[0], linear[1], -linear[2]);
}

void
SGModelPlacement::setBodyAngularVelocity(const SGVec3d& angular)
{
  SGSceneUserData* userData;
  userData = SGSceneUserData::getOrCreateSceneUserData(_transform.get());
  SGSceneUserData::Velocity* vel = userData->getOrCreateVelocity();
  vel->angular = SGVec3d(-angular[0], angular[1], -angular[2]);
}

// end of model.cxx
