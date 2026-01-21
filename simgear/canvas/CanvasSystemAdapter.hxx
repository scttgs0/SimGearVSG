// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Adapter for using the canvas with different applications
 */

#pragma once

#include "canvas_fwd.hxx"

class SGSubsystem;

namespace simgear
{
namespace HTTP { class Client; }
namespace canvas
{

  /**
   * Provides access to different required systems of the application to the
   * Canvas
   */
  class SystemAdapter
  {
    public:

      virtual ~SystemAdapter() {}
      virtual FontPtr getFont(const std::string& name) const = 0;
      virtual void addCamera(vsg::Camera* camera) const = 0;
      virtual void removeCamera(vsg::Camera* camera) const = 0;
      virtual vsg::ref_ptr<vsg::Image> getImage(const std::string& path) const = 0;
      virtual SGSubsystem* getSubsystem(const std::string& name) const = 0;
      virtual HTTP::Client* getHTTPClient() const = 0;
  };

} // namespace canvas
} // namespace simgear
