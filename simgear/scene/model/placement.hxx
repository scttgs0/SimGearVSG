// placement.hxx - manage the placement of a 3D model.
// Written by David Megginson, started 2002.
//
// SPDX-FileCopyrightText: 2002 David Megginson
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Switch>
#include <osg/PositionAttitudeTransform>

#include <simgear/math/SGMath.hxx>


////////////////////////////////////////////////////////////////////////
// Model placement.
////////////////////////////////////////////////////////////////////////

/**
 * A wrapper for a model with a definite placement.
 */
class SGModelPlacement
{
public:

  SGModelPlacement ();
  virtual ~SGModelPlacement ();

  virtual void init( osg::Node* model );
  void clear();
  void add( osg::Node* model );

  virtual void update();

  osg::ref_ptr<osg::Node> getSceneGraph() const;

  virtual bool getVisible () const;
  virtual void setVisible (bool visible);

  void setPosition(const SGGeod& position);
  const SGGeod& getPosition() const { return _position; }

  virtual double getRollDeg () const { return _roll_deg; }
  virtual double getPitchDeg () const { return _pitch_deg; }
  virtual double getHeadingDeg () const { return _heading_deg; }
  SGQuatd getGlobalOrientation() const;

  virtual void setRollDeg (double roll_deg);
  virtual void setPitchDeg (double pitch_deg);
  virtual void setHeadingDeg (double heading_deg);
  virtual void setOrientation (double roll_deg, double pitch_deg,
                               double heading_deg);
  void setOrientation(const SGQuatd& orientation);

  void setReferenceTime(const double& referenceTime);
  void setBodyLinearVelocity(const SGVec3d& velocity);
  void setBodyAngularVelocity(const SGVec3d& velocity);

private:
                                // Geodetic position
  SGGeod _position;

                                // Orientation
  double _roll_deg;
  double _pitch_deg;
  double _heading_deg;

  osg::ref_ptr<osg::Switch> _selector;
  osg::ref_ptr<osg::PositionAttitudeTransform> _transform;
};
