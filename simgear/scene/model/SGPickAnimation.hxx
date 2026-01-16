// SPDX-FileCopyrightText: 2013 James Turner
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <simgear/scene/model/animation.hxx>
#include <simgear/misc/strutils.hxx>

// forward decls
class SGPickCallback;
class SGSceneUserData;

//////////////////////////////////////////////////////////////////////
// Pick animation
//////////////////////////////////////////////////////////////////////

class SGPickAnimation : public SGAnimation {
public:
  SGPickAnimation(simgear::SGTransientModelData &modelData);

  // override so we can treat object-name specially
  void apply(osg::Group& group) override;

  void apply(osg::Node* node);

  protected:
  virtual osg::Group* createMainGroup(osg::Group* pr);
  SGSharedPtr<SGCondition const> _condition;

  virtual void setupCallbacks(SGSceneUserData* ud, osg::Group* parent);

  /**
   * Can the animation be safely repeated / nested without changing the
   * behaviour.
   * If not, then we may have to be careful not to break compatibility with
   * Aircraft which expect brokenness under certain circumstances.
   */
  virtual bool isRepeatable() const;

private:
  class PickCallback;
  class VncCallback;

  string_list _proxyNames;
  std::map<std::string, unsigned int> _objectNamesHandled;
};


class SGKnobAnimation : public SGPickAnimation
{
public:
    SGKnobAnimation(simgear::SGTransientModelData &modelData);

    /**
     * by default mouse wheel up corresponds to increment (CW)
     * and mouse-wheel down corresponds to decrement (CCW).
     * Since no one can agree on that, make it a global toggle.
     */
    static void setAlternateMouseWheelDirection(bool aToggle);

    /**
     * by default mouse is dragged left-right to change knobs.
     * set this to true to default to up-down. Individual knobs
     * can overrider this,
     */
    static void setAlternateDragAxis(bool aToggle);


    /**
     * Scale the drag sensitivity. This provides a global hook for
     * the user to scale the sensitivity of dragging according to
     * personal preference.
     */
    static void setDragSensitivity(double aFactor);


protected:
    virtual osg::Group* createMainGroup(osg::Group* pr);
    SGSharedPtr<SGCondition const> _condition;

    virtual void setupCallbacks(SGSceneUserData* ud, osg::Group* parent);

    bool isRepeatable() const override;

private:
    class UpdateCallback;

    SGVec3d _axis;
    SGVec3d _center;
    SGSharedPtr<SGExpressiond const> _animationValue;
};

class SGSliderAnimation : public SGPickAnimation
{
public:
    SGSliderAnimation(simgear::SGTransientModelData &modelData);


protected:
    virtual osg::Group* createMainGroup(osg::Group* pr);

    virtual void setupCallbacks(SGSceneUserData* ud, osg::Group* parent);

    bool isRepeatable() const override;

private:
    class UpdateCallback;

    SGVec3d _axis;
    SGSharedPtr<SGExpressiond const> _animationValue;
};

class SGTouchAnimation : public SGPickAnimation
{
public:
    SGTouchAnimation(simgear::SGTransientModelData &modelData);


protected:
    virtual osg::Group* createMainGroup(osg::Group* pr);

    virtual void setupCallbacks(SGSceneUserData* ud, osg::Group* parent);

private:
};
