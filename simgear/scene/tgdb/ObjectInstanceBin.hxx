// SPDX-FileCopyrightText: Copyright (C) 2024 Fahim Dalvi
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <vsg/all.h>

#include <simgear/math/SGVec3.hxx>
#include <simgear/math/SGVec4.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>


// these correspond to object-instancing*.eff
const int INSTANCE_POSITIONS = 6;            // (x,y,z)
const int INSTANCE_ROTATIONS_AND_SCALES = 7; // (hdg, pitch, roll, scale)
const int INSTANCE_CUSTOM_ATTRIBS = 10;

namespace simgear {
class ObjectInstanceBin final
{
public:
    struct ObjectInstance {
        // Object with position, rotation scale and customAttribs
        ObjectInstance(const vsg::vec3& p, const vsg::vec3& r = vsg::vec3(0.0f, 0.0f, 0.0f), const float& s = 1.0f, const vsg::vec4& c = vsg::vec4(0.0f, 0.0f, 0.0f, 0.0f)) : position(p), rotation(r), scale(s), customAttribs(c) {}

        vsg::vec3 position;
        vsg::vec3 rotation; // hdg, pitch, roll
        float scale;
        vsg::vec4 customAttribs;
    };

    typedef std::vector<ObjectInstance> ObjectInstanceList;

    ObjectInstanceBin() = default;
    ObjectInstanceBin(const std::string modelFileName, const std::string effect = "default",
                      const SGPath& STGFilePath = SGPath{std::string{"dynamically-generated"}},
                      const SGPath& instancesFilePath = SGPath{});

    ~ObjectInstanceBin() = default; // non-virtual intentional

    void insert(const ObjectInstance& light);
    void insert(const vsg::vec3& p, const vsg::vec3& r = vsg::vec3(0.0f, 0.0f, 0.0f), const float& s = 1.0f, const vsg::vec4& c = vsg::vec4(0.0f, 0.0f, 0.0f, 0.0f));

    const std::string getModelFileName() const;
    const SGPath getSTGFilePath() const;
    const std::string getEffect() const;
    unsigned getNumInstances() const;
    const ObjectInstance& getInstance(unsigned i) const;
    bool hasCustomAttributes() const;

private:
    SGPath _STGFilePath;
    std::string _modelFileName;
    std::string _effect;
    ObjectInstanceList _objectInstances;

    // List of effects that take extra custom attributes
    const std::set<std::string> customInstancingEffects = {"Effects/object-instancing-colored"};
};

vsg::ref_ptr<vsg::Node> createObjectInstances(ObjectInstanceBin& objectInstances, const vsg::mat4& transform, const vsg::ref_ptr<SGReaderWriterOptions> options);
}; // namespace simgear
