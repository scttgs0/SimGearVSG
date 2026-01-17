// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#pragma once

#include <simgear/structure/SGReferenced.hxx>

namespace simgear {

class BVHStaticData;
class BVHVisitor;

class BVHStaticNode : public SGReferenced {
public:
  virtual ~BVHStaticNode();

  virtual void accept(BVHVisitor&, const BVHStaticData&) const = 0;
};

}
