// Copyright (C) 2018 - 2023 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <unordered_map>
#include <vector>
#include <array>

// For osgUtil::LineSegmentIntersector::Intersections, which is a typedef.
#include <osgUtil/LineSegmentIntersector>

#include "CompositorBuffer.hxx"
#include "CompositorPass.hxx"

class SGPropertyNode;

namespace simgear::compositor {

/**
 * A Compositor manages the rendering pipeline of a single physical camera,
 * usually via a property tree interface.
 *
 * The building blocks that define a Compositor are:
 *   - Buffers. They represent a zone of GPU memory. This is implemented in the
 *     form of an OpenGL texture, but any type of information can be stored
 *     (which can be useful in compute shaders for example).
 *   - Passes. They represent render operations. They can get buffers as input
 *     and they can output to other buffers. They are also integrated with the
 *     Effects framework, so the OpenGL internal state is configurable per pass.
 */
class Compositor final {
public:
    enum BuiltinUniform {
        SG_UNIFORM_TEXTURE_MATRIX = 0,
        SG_UNIFORM_VIEWPORT,
        SG_UNIFORM_PIXEL_SIZE,
        SG_UNIFORM_ASPECT_RATIO,
        SG_UNIFORM_VIEW_MATRIX,
        SG_UNIFORM_VIEW_MATRIX_INV,
        SG_UNIFORM_PROJECTION_MATRIX,
        SG_UNIFORM_PROJECTION_MATRIX_INV,
        SG_UNIFORM_PREV_VIEW_MATRIX,
        SG_UNIFORM_PREV_VIEW_MATRIX_INV,
        SG_UNIFORM_PREV_PROJECTION_MATRIX,
        SG_UNIFORM_PREV_PROJECTION_MATRIX_INV,
        SG_UNIFORM_CAMERA_POSITION_CART,
        SG_UNIFORM_CAMERA_POSITION_GEOD,
        SG_UNIFORM_CAMERA_DISTANCE_TO_EARTH_CENTER,
        SG_UNIFORM_CAMERA_WORLD_UP,
        SG_UNIFORM_CAMERA_VIEW_UP,
        SG_UNIFORM_NEAR_FAR,
        SG_UNIFORM_FCOEF,
        SG_UNIFORM_FOV_SCALE,
        SG_UNIFORM_FOV_CENTER,
        SG_UNIFORM_SUN_DIRECTION,
        SG_UNIFORM_SUN_DIRECTION_WORLD,
        SG_UNIFORM_SUN_ZENITH_COSTHETA,
        SG_UNIFORM_MOON_DIRECTION,
        SG_UNIFORM_MOON_DIRECTION_WORLD,
        SG_UNIFORM_MOON_ZENITH_COSTHETA,
        SG_UNIFORM_EARTH_RADIUS,
        SG_TOTAL_BUILTIN_UNIFORMS
    };

    struct MVRInfo {
        unsigned int views = 1;
        std::string viewIdGlobalStr = "";
        std::string viewIdStr[3] = {"0", "0", "0"};
        unsigned int cells = 1;
    };

    Compositor(osg::View *view,
               osg::GraphicsContext *gc,
               osg::Viewport *viewport,
               const MVRInfo *mvr_info = nullptr);
    ~Compositor();

    /**
     * \brief Create a Compositor from a property tree.
     *
     * @param view The View where the passes will be added as slaves.
     * @param gc The context where the internal osg::Cameras will draw on.
     * @param viewport The viewport position and size inside the window.
     * @param property_list A valid property list that describes the Compositor.
     * @param mvr_info Multiview rendering information.
     * @return A Compositor or a null pointer if there was an error.
     */
    static Compositor *create(osg::View *view,
                              osg::GraphicsContext *gc,
                              osg::Viewport *viewport,
                              const SGPropertyNode *property_list,
                              const SGReaderWriterOptions *options,
                              const MVRInfo *mvr_info = nullptr);
    /**
     * \overload
     * \brief Create a Compositor from a file.
     *
     * @param name Name of the compositor. The function will search for a file
     *             named <name>.xml in $FG_ROOT.
     */
    static Compositor *create(osg::View *view,
                              osg::GraphicsContext *gc,
                              osg::Viewport *viewport,
                              const std::string &name,
                              const SGReaderWriterOptions *options,
                              const MVRInfo *mvr_info = nullptr);

    static SGPropertyNode_ptr loadPropertyList(const std::string &name);

    void updateSubView(unsigned int sub_view_index,
                       const vsg::mat4 &view_matrix,
                       const vsg::mat4 &proj_matrix,
                       const vsg::vec4 &viewport);

    void               update(const vsg::mat4 &view_matrix,
                              const vsg::mat4 &proj_matrix);

    void               resized();

    void               setCullMask(vsg::Node::NodeMask cull_mask);

    void               setLODScale(float scale);

    osg::View         *getView() const { return _view; }

    osg::GraphicsContext *getGraphicsContext() const { return _gc; }

    osg::Viewport     *getViewport() const { return _viewport; }

    typedef std::array<
        vsg::ref_ptr<osg::Uniform>,
        SG_TOTAL_BUILTIN_UNIFORMS> BuiltinUniforms;
    const BuiltinUniforms &getBuiltinUniforms() const { return _uniforms; }

    void               addBuffer(const std::string &name, Buffer *buffer);
    void               addPass(Pass *pass);

    void               setName(const std::string &name) { _name = name; }
    const std::string &getName() const { return _name; }

    unsigned int getMVRViews() const { return _mvr.views; }
    const std::string &getMVRViewIdGlobalStr() const { return _mvr.viewIdGlobalStr; }
    const std::string &getMVRViewIdStr(unsigned int index) const { return _mvr.viewIdStr[index]; }
    unsigned int getMVRCells() const { return _mvr.cells; }

    typedef std::unordered_map<std::string, vsg::ref_ptr<Buffer>> BufferMap;
    const BufferMap &  getBufferMap() const { return _buffers; }
    Buffer *           getBuffer(const std::string &name) const;

    typedef std::vector<vsg::ref_ptr<Pass>> PassList;
    const PassList &   getPassList() const { return _passes; }
    unsigned int       getNumPasses() const { return _passes.size(); }
    Pass *             getPass(size_t index) const { return _passes[index]; }
    Pass *             getPass(const std::string &name) const;

    int                getOrderOffset() const { return _order_offset; }
    static void        resetOrderOffset() { _order_offset = 0; }
protected:
    osg::View                   *_view;
    osg::GraphicsContext        *_gc;
    vsg::ref_ptr<osg::Viewport>  _viewport;
    std::string                  _name;
    MVRInfo                      _mvr;
    BufferMap                    _buffers;
    PassList                     _passes;
    BuiltinUniforms              _uniforms;
    static int                   _order_offset;
};

} // namespace simgear::compositor
