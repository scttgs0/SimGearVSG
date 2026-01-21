// Copyright (C) 2023 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

/**
 * @brief Utility functions for manipulating projection matrices.
 *
 * They allow transparent handling of both standard OpenGL and reverse depth
 * projection matrices. It is recommended to always use these functions to
 * obtain a projection matrix and not OSG's own functions for consistent
 * behaviour.
 *
 * Based on osgEarth's handling of projection matrices and some math borrowed
 * from GLM (OpenGL Mathematics).
 */

#pragma once

#include <osg/Matrixd>

namespace simgear::ProjectionMatrix {

enum Type {
    STANDARD,
    REVERSE_DEPTH_ZO,
    REVERSE_DEPTH_NO
};

bool isOrtho(const vsg::dmat4 &m);

bool isPerspective(const vsg::dmat4 &m);

Type getType(const vsg::dmat4 &m);

void makePerspective(vsg::dmat4 &m,
                     double vfov, double aspect, double near, double far,
                     Type type = STANDARD);

void makeFrustum(vsg::dmat4 &m,
                 double left, double right,
                 double bottom, double top,
                 double near, double far,
                 Type type = STANDARD);

void makeOrtho(vsg::dmat4 &m,
               double left, double right,
               double bottom, double top,
               double near, double far,
               Type type = STANDARD);

bool getFrustum(const vsg::dmat4 &m,
                double &left, double &right,
                double &bottom, double &top,
                double &near, double &far);

bool getOrtho(const vsg::dmat4 &m,
              double &left, double &right,
              double &bottom, double &top,
              double &near, double &far);

/**
 * Given a projection matrix, return a new one with the same frustum sides and
 * new near/far planes.
 */
void clampNearFarPlanes(vsg::dmat4 &old_proj, double near, double far,
                        vsg::dmat4 &new_proj);

} // namespace simgear::ProjectionMatrix
