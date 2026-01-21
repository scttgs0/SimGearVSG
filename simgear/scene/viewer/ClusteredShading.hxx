// Copyright (C) 2018 - 2023 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <atomic>

#include <vsg/all.h>

#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/Uniform>

#include <simgear/scene/model/SGLight.hxx>


namespace simgear::compositor {

class ClusteredShading final : public osg::Referenced
{
public:
    ClusteredShading(vsg::Camera* camera, const SGPropertyNode* config);
    ~ClusteredShading();

    void exposeUniformsToPass(vsg::Camera* camera,
                              int clusters_bind_unit,
                              int indices_bind_unit,
                              int pointlights_bind_unit,
                              int spotlights_bind_unit);
    void update(const SGLightList& light_list);

protected:
    // We could make use of osg::Polytope, but it does a lot of std::vector
    // push_back() calls, so we make our own frustum structure for huge
    // performance gains.
    struct Subfrustum {
        vsg::vec4 plane[6];
    };

    struct PointlightBound {
        SGLight* light = nullptr;
        vsg::vec4 position;
        float range = 0.0f;
    };
    struct SpotlightBound {
        SGLight* light = nullptr;
        vsg::vec4 position;
        vsg::vec4 direction;
        float cos_cutoff = 0.0f;
        struct {
            vsg::vec4 center;
            float radius = 0.0f;
        } bounding_sphere;
    };

    void updateLightBounds(const SGLightList& light_list);
    void sortLightBounds();
    void recreateSubfrustaIfNeeded();
    void updateUniforms();
    void updateSubfrusta();
    void threadFunc(int thread_id);
    void assignLightsToSlice(int slice);
    void writePointlightData();
    void writeSpotlightData();
    void writePointlightDataPBR();
    void writeSpotlightDataPBR();
    float getDepthForSlice(int slice) const;

    osg::observer_ptr<vsg::Camera> _camera;

    vsg::ref_ptr<osg::Uniform> _slice_scale;
    vsg::ref_ptr<osg::Uniform> _slice_bias;
    vsg::ref_ptr<osg::Uniform> _horizontal_tiles;
    vsg::ref_ptr<osg::Uniform> _vertical_tiles;

    bool _pbr_lights = false;

    int _max_pointlights = 0;
    int _max_spotlights = 0;
    int _max_light_indices = 0;
    int _tile_size = 0;
    int _depth_slices = 0;
    int _num_threads = 0;
    int _slices_per_thread = 0;
    int _slices_remainder = 0;

    float _zNear = 0.0f;
    float _zFar = 0.0f;

    int _old_width = 0;
    int _old_height = 0;

    int _n_htiles = 0;
    int _n_vtiles = 0;

    float _x_step = 0.0f;
    float _y_step = 0.0f;

    vsg::ref_ptr<vsg::Image> _clusters;
    vsg::ref_ptr<vsg::Image> _indices;
    vsg::ref_ptr<vsg::Image> _pointlights;
    vsg::ref_ptr<vsg::Image> _spotlights;

    vsg::ref_ptr<osg::Texture3D> _clusters_tex;
    vsg::ref_ptr<osg::Texture2D> _indices_tex;
    vsg::ref_ptr<osg::Texture2D> _pointlights_tex;
    vsg::ref_ptr<osg::Texture2D> _spotlights_tex;

    std::unique_ptr<Subfrustum[]> _subfrusta;

    std::vector<PointlightBound> _point_bounds;
    std::vector<SpotlightBound> _spot_bounds;

    std::atomic<int> _global_light_count;
};

} // namespace simgear::compositor
