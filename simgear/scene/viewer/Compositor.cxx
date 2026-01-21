// Copyright (C) 2018 - 2023 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "Compositor.hxx"

#include <algorithm>

#include <osg/DispatchCompute>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture2DMultisample>
#include <osg/Texture3D>
#include <osg/TextureRectangle>
#include <osg/TextureCubeMap>
#include <osgUtil/IntersectionVisitor>

#include <simgear/math/SGRect.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/structure/exception.hxx>

#include "CompositorUtil.hxx"

class SunDirectionWorldCallback : public osg::Uniform::Callback {
public:
    virtual void operator()(osg::Uniform *uniform, osg::NodeVisitor *nv) {
        assert(dynamic_cast<SGUpdateVisitor*>(nv));
        SGUpdateVisitor* uv = static_cast<SGUpdateVisitor*>(nv);
        vsg::vec3 l = toOsg(uv->getLightDirection());
        l.normalize();
        uniform->set(l);
    }
};

class MoonDirectionWorldCallback : public osg::Uniform::Callback {
public:
    virtual void operator()(osg::Uniform *uniform, osg::NodeVisitor *nv) {
        assert(dynamic_cast<SGUpdateVisitor*>(nv));
        SGUpdateVisitor* uv = static_cast<SGUpdateVisitor*>(nv);
        vsg::vec3 l = toOsg(uv->getSecondLightDirection());
        l.normalize();
        uniform->set(l);
    }
};

namespace simgear::compositor {

int Compositor::_order_offset = 0;

Compositor *
Compositor::create(osg::View *view,
                   osg::GraphicsContext *gc,
                   osg::Viewport *viewport,
                   const SGPropertyNode *property_list,
                   const SGReaderWriterOptions *options,
                   const Compositor::MVRInfo *mvr_info)
{
    Compositor *compositor = new Compositor(view, gc, viewport, mvr_info);
    compositor->_name = property_list->getStringValue("name");

    gc->getState()->setUseModelViewAndProjectionUniforms(
        property_list->getBoolValue("use-osg-uniforms", false));
    gc->getState()->setUseVertexAttributeAliasing(
        property_list->getBoolValue("use-vertex-attribute-aliasing", false));

    // Read all buffers first so passes can use them
    PropertyList p_buffers = property_list->getChildren("buffer");
    for (auto const &p_buffer : p_buffers) {
        if (!checkConditional(p_buffer))
            continue;
        const std::string &buffer_name = p_buffer->getStringValue("name");
        if (buffer_name.empty()) {
            SG_LOG(SG_INPUT, SG_ALERT, "Compositor::build: Buffer requires "
                   "a name to be available to passes. Skipping...");
            continue;
        }
        Buffer *buffer = buildBuffer(compositor, p_buffer, options);
        if (buffer)
            compositor->addBuffer(buffer_name, buffer);
    }
    // Read passes
    PropertyList p_passes = property_list->getChildren("pass");
    for (auto const &p_pass : p_passes) {
        if (!checkConditional(p_pass))
            continue;
        Pass *pass = buildPass(compositor, p_pass, options);
        if (pass)
            compositor->addPass(pass);
    }

    ++_order_offset;

    return compositor;
}

Compositor *
Compositor::create(osg::View *view,
                   osg::GraphicsContext *gc,
                   osg::Viewport *viewport,
                   const std::string &name,
                   const SGReaderWriterOptions *options,
                   const Compositor::MVRInfo *mvr_info)
{
    SGPropertyNode_ptr property_list = loadPropertyList(name);
    if (!property_list.valid())
        return nullptr;
    return create(view, gc, viewport, property_list, options, mvr_info);
}

SGPropertyNode_ptr Compositor::loadPropertyList(const std::string &name)
{
    std::string filename(name);
    filename += ".xml";
    std::string abs_filename = SGModelLib::findDataFile(filename);
    if (abs_filename.empty()) {
        SG_LOG(SG_INPUT, SG_ALERT, "Compositor::build: Could not find file '"
               << filename << "'");
        return nullptr;
    }

    SGPropertyNode_ptr property_list = new SGPropertyNode;
    try {
        readProperties(abs_filename, property_list.ptr(), 0, true);
        return property_list;
    } catch (sg_io_exception &e) {
        SG_LOG(SG_INPUT, SG_ALERT, "Compositor::build: Failed to parse file '"
               << abs_filename << "'. " << e.getFormattedMessage());
        return nullptr;
    }
}

Compositor::Compositor(osg::View *view,
                       osg::GraphicsContext *gc,
                       osg::Viewport *viewport,
                       const Compositor::MVRInfo *mvr_info) :
    _view(view),
    _gc(gc),
    _viewport(viewport),
    _mvr{ mvr_info ? mvr_info->views : 1 },
    _uniforms{
    new osg::Uniform("fg_TextureMatrix", vsg::mat4()),
    new osg::Uniform(osg::Uniform::FLOAT_VEC4, "fg_Viewport", _mvr.views),
    new osg::Uniform("fg_PixelSize", vsg::vec2()),
    new osg::Uniform("fg_AspectRatio", 0.0f),
    new osg::Uniform(osg::Uniform::FLOAT_MAT4, "fg_ViewMatrix", _mvr.views),
    new osg::Uniform(osg::Uniform::FLOAT_MAT4, "fg_ViewMatrixInverse", _mvr.views),
    new osg::Uniform(osg::Uniform::FLOAT_MAT4, "fg_ProjectionMatrix", _mvr.views),
    new osg::Uniform(osg::Uniform::FLOAT_MAT4, "fg_ProjectionMatrixInverse", _mvr.views),
    new osg::Uniform("fg_PrevViewMatrix", vsg::mat4()),
    new osg::Uniform("fg_PrevViewMatrixInverse", vsg::mat4()),
    new osg::Uniform("fg_PrevProjectionMatrix", vsg::mat4()),
    new osg::Uniform("fg_PrevProjectionMatrixInverse", vsg::mat4()),
    new osg::Uniform("fg_CameraPositionCart", vsg::vec3()),
    new osg::Uniform("fg_CameraPositionGeod", vsg::vec3()),
    new osg::Uniform("fg_CameraDistanceToEarthCenter", 0.0f),
    new osg::Uniform("fg_CameraWorldUp", vsg::vec3()),
    new osg::Uniform(osg::Uniform::FLOAT_VEC3, "fg_CameraViewUp", _mvr.views),
    new osg::Uniform("fg_NearFar", vsg::vec2()),
    new osg::Uniform("fg_Fcoef", 0.0f),
    new osg::Uniform(osg::Uniform::FLOAT_VEC2, "fg_FOVScale", _mvr.views),
    new osg::Uniform(osg::Uniform::FLOAT_VEC2, "fg_FOVCenter", _mvr.views),
    new osg::Uniform(osg::Uniform::FLOAT_VEC3, "fg_SunDirection", _mvr.views),
    new osg::Uniform("fg_SunDirectionWorld", vsg::vec3()),
    new osg::Uniform("fg_SunZenithCosTheta", 0.0f),
    new osg::Uniform(osg::Uniform::FLOAT_VEC3, "fg_MoonDirection", _mvr.views),
    new osg::Uniform("fg_MoonDirectionWorld", vsg::vec3()),
    new osg::Uniform("fg_MoonZenithCosTheta", 0.0f),
    new osg::Uniform("fg_EarthRadius", 0.0f),
    }
{
    if (mvr_info) {
        _mvr = *mvr_info;
    }
    _uniforms[SG_UNIFORM_SUN_DIRECTION_WORLD]->setUpdateCallback(
        new SunDirectionWorldCallback);
    _uniforms[SG_UNIFORM_MOON_DIRECTION_WORLD]->setUpdateCallback(
        new MoonDirectionWorldCallback);
}

Compositor::~Compositor()
{
    // Remove slave cameras from the viewer
    for (const auto &pass : _passes) {
        vsg::Camera *camera = pass->camera;
        // Remove all children before removing the slave to prevent the graphics
        // window from automatically cleaning up all associated OpenGL objects.
        camera->removeChildren(0, camera->getNumChildren());

        unsigned int index = _view->findSlaveIndexForCamera(camera);
        _view->removeSlave(index);
    }
}

void Compositor::updateSubView(unsigned int sub_view_index,
                               const vsg::mat4 &view_matrix,
                               const vsg::mat4 &proj_matrix,
                               const vsg::vec4 &viewport)
{
    for (auto &pass : _passes) {
        if (pass->update_callback.valid())
            pass->update_callback->updateSubView(*pass.get(), sub_view_index, view_matrix, proj_matrix);
    }

    // Update uniforms
    vsg::dmat4 view_inverse = vsg::mat4::inverse(view_matrix);
    vsg::dvec4 camera_pos4 = vsg::dvec4(0.0, 0.0, 0.0, 1.0) * view_inverse;
    vsg::dvec3 camera_pos = vsg::dvec3(camera_pos4.x(),
                                       camera_pos4.y(),
                                       camera_pos4.z());

    vsg::dvec3 world_up = camera_pos;
    world_up.normalize();
    vsg::dvec3 view_up = world_up * view_matrix;
    view_up.normalize();

    double left = -1.0, right = 1.0, bottom = -1.0, top = 1.0,
           zNear = -1.0, zFar = 1.0;
    proj_matrix.getFrustum(left, right, bottom, top, zNear, zFar);

    _uniforms[SG_UNIFORM_VIEWPORT]->setElement(sub_view_index,
                                               vsg::vec4(viewport.x(),
                                                          viewport.y(),
                                                          viewport.z(),
                                                          viewport.w()));
    _uniforms[SG_UNIFORM_VIEW_MATRIX]->setElement(sub_view_index, view_matrix);
    _uniforms[SG_UNIFORM_VIEW_MATRIX_INV]->setElement(sub_view_index, view_inverse);
    _uniforms[SG_UNIFORM_PROJECTION_MATRIX]->setElement(sub_view_index, proj_matrix);
    _uniforms[SG_UNIFORM_PROJECTION_MATRIX_INV]->setElement(sub_view_index, vsg::mat4::inverse(proj_matrix));

    _uniforms[SG_UNIFORM_CAMERA_VIEW_UP]->setElement(sub_view_index, vsg::vec3(view_up));

    float aspect_ratio = proj_matrix(1, 1) / proj_matrix(0, 0);
    float tan_fov_y = 1.0f / proj_matrix(1, 1);
    float tan_fov_x = tan_fov_y * aspect_ratio;
    // The forward vector UV coordinate may not be at 0.5 due to side-by-side
    // multiview viewports, and also asymmetric FOV (especially for VR HMDs).
    if (_mvr.views > 1) {
        _uniforms[SG_UNIFORM_FOV_SCALE]->setElement(sub_view_index, vsg::vec2(
            tan_fov_x * _viewport->width() / viewport.z(),
            tan_fov_y * _viewport->height() / viewport.w()) * 2.0f);
        _uniforms[SG_UNIFORM_FOV_CENTER]->setElement(sub_view_index, vsg::vec2(
            (viewport.x() + viewport.z() * (-left / (right - left))) / _viewport->width(),
            (viewport.y() + viewport.w() * (-bottom / (top - bottom))) / _viewport->height()));
    } else {
        _uniforms[SG_UNIFORM_FOV_SCALE]->setElement(sub_view_index, vsg::vec2(
            tan_fov_x,
            tan_fov_y) * 2.0f);
        _uniforms[SG_UNIFORM_FOV_CENTER]->setElement(sub_view_index, vsg::vec2(
            -left / (right - left),
            -bottom / (top - bottom)));
    }

    vsg::vec3 sun_dir_world;
    _uniforms[SG_UNIFORM_SUN_DIRECTION_WORLD]->get(sun_dir_world);
    vsg::vec4 sun_dir_view = vsg::vec4(
        sun_dir_world.x(), sun_dir_world.y(), sun_dir_world.z(), 0.0f) * view_matrix;
    _uniforms[SG_UNIFORM_SUN_DIRECTION]->setElement(sub_view_index, vsg::vec3(sun_dir_view.x(), sun_dir_view.y(), sun_dir_view.z()));

    vsg::vec3 moon_dir_world;
    _uniforms[SG_UNIFORM_MOON_DIRECTION_WORLD]->get(moon_dir_world);
    vsg::vec4 moon_dir_view = vsg::vec4(
        moon_dir_world.x(), moon_dir_world.y(), moon_dir_world.z(), 0.0f) * view_matrix;
    _uniforms[SG_UNIFORM_MOON_DIRECTION]->setElement(sub_view_index, vsg::vec3(moon_dir_view.x(), moon_dir_view.y(), moon_dir_view.z()));
}

void
Compositor::update(const vsg::mat4 &view_matrix,
                   const vsg::mat4 &proj_matrix)
{
    // Enable/disable passes by setting or unsetting their graphics context.
    // XXX: Check if this causes threading-related crashes.
    // Also run the update callback for enabled passes.
    for (auto &pass : _passes) {
        vsg::Camera* camera = pass->camera;
        bool should_render = (!pass->render_condition || pass->render_condition->test())
            && (!pass->render_once || !pass->has_ever_rendered);
        if (should_render) {
            // Pass is enabled
            camera->setNodeMask(0xffffffff);
            if (pass->update_callback.valid()) {
                pass->update_callback->updatePass(*pass.get(), view_matrix, proj_matrix);
            }
            pass->has_ever_rendered = true;
        } else {
            // Pass is disabled
            camera->setNodeMask(0);
        }

    }

    // Update uniforms
    vsg::dmat4 view_inverse = vsg::mat4::inverse(view_matrix);
    vsg::dvec4 camera_pos4 = vsg::dvec4(0.0, 0.0, 0.0, 1.0) * view_inverse;
    vsg::dvec3 camera_pos = vsg::dvec3(camera_pos4.x(),
                                       camera_pos4.y(),
                                       camera_pos4.z());
    SGGeod camera_pos_geod = SGGeod::fromCart(
        SGVec3d(camera_pos.x(), camera_pos.y(), camera_pos.z()));

    vsg::dvec3 world_up = camera_pos;
    world_up.normalize();
    vsg::dvec3 view_up = world_up * view_matrix;
    view_up.normalize();

    double left = 0.0, right = 0.0, bottom = 0.0, top = 0.0,
        zNear = 0.0, zFar = 0.0;
    proj_matrix.getFrustum(left, right, bottom, top, zNear, zFar);

    vsg::mat4 prev_view_matrix, prev_view_matrix_inv;
    _uniforms[SG_UNIFORM_VIEW_MATRIX]->getElement(0, prev_view_matrix);
    _uniforms[SG_UNIFORM_VIEW_MATRIX_INV]->getElement(0, prev_view_matrix_inv);
    vsg::mat4 prev_proj_matrix, prev_proj_matrix_inv;
    _uniforms[SG_UNIFORM_PROJECTION_MATRIX]->get(prev_proj_matrix);
    _uniforms[SG_UNIFORM_PROJECTION_MATRIX_INV]->get(prev_proj_matrix_inv);

    _uniforms[SG_UNIFORM_PREV_VIEW_MATRIX]->set(prev_view_matrix);
    _uniforms[SG_UNIFORM_PREV_VIEW_MATRIX_INV]->set(prev_view_matrix_inv);
    _uniforms[SG_UNIFORM_PREV_PROJECTION_MATRIX]->set(prev_proj_matrix);
    _uniforms[SG_UNIFORM_PREV_PROJECTION_MATRIX_INV]->set(prev_proj_matrix_inv);

    vsg::vec3 sun_dir_world;
    _uniforms[SG_UNIFORM_SUN_DIRECTION_WORLD]->get(sun_dir_world);
    vsg::vec4 sun_dir_view = vsg::vec4(
        sun_dir_world.x(), sun_dir_world.y(), sun_dir_world.z(), 0.0f) * view_matrix;

    vsg::vec3 moon_dir_world;
    _uniforms[SG_UNIFORM_MOON_DIRECTION_WORLD]->get(moon_dir_world);
    vsg::vec4 moon_dir_view = vsg::vec4(
        moon_dir_world.x(), moon_dir_world.y(), moon_dir_world.z(), 0.0f) * view_matrix;

    float aspect_ratio = proj_matrix(1,1) / proj_matrix(0,0);
    float tan_fov_y = 1.0f / proj_matrix(1,1);
    float tan_fov_x = tan_fov_y * aspect_ratio;

    for (int i = 0; i < SG_TOTAL_BUILTIN_UNIFORMS; ++i) {
        vsg::ref_ptr<osg::Uniform> u = _uniforms[i];
        switch (i) {
        case SG_UNIFORM_VIEW_MATRIX:
            u->setElement(0, view_matrix);
            break;
        case SG_UNIFORM_VIEW_MATRIX_INV:
            u->setElement(0, view_inverse);
            break;
        case SG_UNIFORM_PROJECTION_MATRIX:
            u->setElement(0, proj_matrix);
            break;
        case SG_UNIFORM_PROJECTION_MATRIX_INV:
            u->setElement(0, vsg::mat4::inverse(proj_matrix));
            break;
        case SG_UNIFORM_CAMERA_POSITION_CART:
            u->set(vsg::vec3(camera_pos));
            break;
        case SG_UNIFORM_CAMERA_POSITION_GEOD:
            u->set(vsg::vec3(camera_pos_geod.getLongitudeRad(),
                              camera_pos_geod.getLatitudeRad(),
                              camera_pos_geod.getElevationM()));
            break;
        case SG_UNIFORM_CAMERA_DISTANCE_TO_EARTH_CENTER:
            u->set(float(camera_pos.length()));
            break;
        case SG_UNIFORM_CAMERA_WORLD_UP:
            u->set(vsg::vec3(world_up));
            break;
        case SG_UNIFORM_CAMERA_VIEW_UP:
            u->setElement(0, vsg::vec3(view_up));
            break;
        case SG_UNIFORM_NEAR_FAR:
            u->set(vsg::vec2(zNear, zFar));
            break;
        case SG_UNIFORM_FCOEF:
            u->set(float(2.0 / log2(zFar + 1.0)));
            break;
        case SG_UNIFORM_FOV_SCALE:
            u->setElement(0, vsg::vec2(tan_fov_x, tan_fov_y) * 2.0f);
            break;
        case SG_UNIFORM_FOV_CENTER:
            u->setElement(0, vsg::vec2(-left / (right - left),
                                        -bottom / (top - bottom)));
            break;
        case SG_UNIFORM_SUN_DIRECTION:
            u->setElement(0, vsg::vec3(sun_dir_view.x(), sun_dir_view.y(), sun_dir_view.z()));
            break;
        case SG_UNIFORM_SUN_ZENITH_COSTHETA:
            u->setElement(0, float(sun_dir_world * world_up));
            break;
        case SG_UNIFORM_MOON_DIRECTION:
            u->setElement(0, vsg::vec3(moon_dir_view.x(), moon_dir_view.y(), moon_dir_view.z()));
            break;
        case SG_UNIFORM_MOON_ZENITH_COSTHETA:
            u->set(float(moon_dir_world * world_up));
            break;
        case SG_UNIFORM_EARTH_RADIUS:
            u->set(float(camera_pos.length() - camera_pos_geod.getElevationM()));
            break;
        default:
            // Unknown uniform
            break;
        }
    }
}

void
Compositor::resized()
{
    // Cameras attached directly to the framebuffer were already resized by
    // osg::GraphicsContext::resizedImplementation(). However, RTT cameras were
    // ignored. Here we resize RTT cameras that need to match the physical
    // viewport size.
    for (const auto &pass : _passes) {
        vsg::Camera *camera = pass->camera;
        if (!camera)
            continue;

        osg::Viewport *viewport = camera->getViewport();

        if (camera->isRenderToTextureCamera() &&
            (pass->viewport_x_scale      != 0.0f ||
             pass->viewport_y_scale      != 0.0f ||
             pass->viewport_width_scale  != 0.0f ||
             pass->viewport_height_scale != 0.0f)) {

            // Resize the viewport
            int new_x = (pass->viewport_x_scale == 0.0f) ?
                viewport->x() :
                pass->viewport_x_scale * _viewport->width();
            int new_y = (pass->viewport_y_scale == 0.0f) ?
                viewport->y() :
                pass->viewport_y_scale * _viewport->height();
            int new_width = (pass->viewport_width_scale == 0.0f) ?
                viewport->width() :
                pass->viewport_width_scale * _viewport->width();
            int new_height = (pass->viewport_height_scale == 0.0f) ?
                viewport->height() :
                pass->viewport_height_scale * _viewport->height();
            camera->setViewport(new_x, new_y, new_width, new_height);

            // Force the OSG rendering backend to handle the new sizes
            camera->dirtyAttachmentMap();
        }

        // Resize any compute dispatch related to screen size
        if (pass->compute_node.valid() &&
            (pass->compute_global_scale[0] != 0.0f ||
             pass->compute_global_scale[1] != 0.0f)) {
            auto* computeNode = static_cast<osg::DispatchCompute*>(pass->compute_node.get());
            vsg::vec2 screenSize(_viewport->width(), _viewport->height());
            osg::Vec3i groups;
            computeNode->getComputeGroups(groups[0], groups[1], groups[2]);
            for (int dim = 0; dim < 2; ++dim) {
                if (pass->compute_global_scale[dim] != 0.0f) {
                    // resize this dimension
                    groups[dim] = (int)ceil(ceil(screenSize[dim] * pass->compute_global_scale[dim]) / pass->compute_wg_size[dim]);
                    if (groups[dim] < 1)
                        groups[dim] = 1;
                }
            }
            computeNode->setComputeGroups(groups[0], groups[1], groups[2]);
        }

        // Update the uniforms even if it isn't a RTT camera
        _uniforms[SG_UNIFORM_VIEWPORT]->setElement(0,
            vsg::vec4(viewport->x(),
                       viewport->y(),
                       viewport->width(),
                       viewport->height()));
        _uniforms[SG_UNIFORM_PIXEL_SIZE]->set(
            vsg::vec2(1.0f / viewport->width(),
                       1.0f / viewport->height()));
        _uniforms[SG_UNIFORM_ASPECT_RATIO]->set(
            float(viewport->width() / viewport->height()));
    }

    // Resize buffers that must be a multiple of the screen size
    for (const auto &buffer : _buffers) {
        osg::Texture *texture = buffer.second->texture;
        if (texture &&
            (buffer.second->width_scale  != 0.0f ||
             buffer.second->height_scale != 0.0f)) {
            {
                auto tex = dynamic_cast<osg::Texture1D *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    tex->setTextureWidth(new_width);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture2D *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture2DArray *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height, tex->getTextureDepth());
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture2DMultisample *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::Texture3D *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height, tex->getTextureDepth());
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::TextureRectangle *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height);
                    tex->dirtyTextureObject();
                }
            }
            {
                auto tex = dynamic_cast<osg::TextureCubeMap *>(texture);
                if (tex) {
                    int new_width = (buffer.second->width_scale == 0.0f) ?
                        tex->getTextureWidth() :
                        buffer.second->width_scale * _viewport->width();
                    int new_height = (buffer.second->height_scale == 0.0f) ?
                        tex->getTextureHeight() :
                        buffer.second->height_scale * _viewport->height();
                    tex->setTextureSize(new_width, new_height);
                    tex->dirtyTextureObject();
                }
            }
        }
    }
}

void
Compositor::setCullMask(vsg::Node::NodeMask cull_mask)
{
    for (auto &pass : _passes) {
        vsg::Camera *camera = pass->camera;
        if (pass->inherit_cull_mask) {
            camera->setCullMask(pass->cull_mask & cull_mask);
            camera->setCullMaskLeft(pass->cull_mask & cull_mask & ~RIGHT_BIT);
            camera->setCullMaskRight(pass->cull_mask & cull_mask & ~LEFT_BIT);
        } else {
            camera->setCullMask(pass->cull_mask);
            camera->setCullMaskLeft(pass->cull_mask & ~RIGHT_BIT);
            camera->setCullMaskRight(pass->cull_mask & ~LEFT_BIT);
        }
    }
}

void
Compositor::setLODScale(float scale)
{
    for (auto &pass: _passes) {
        // Only change the LOD scale for passes that actually render the scene
        // and do not have a custom scale.
        if (pass->useMastersSceneData && !pass->has_custom_lod_scale) {
            pass->camera->setLODScale(scale);
        }
    }
}

void
Compositor::addBuffer(const std::string &name, Buffer *buffer)
{
    _buffers[name] = buffer;
}

void
Compositor::addPass(Pass *pass)
{
    if (!_view) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Compositor::addPass: Couldn't add camera "
               "as a slave to the view. View doesn't exist!");
        return;
    }
    _view->addSlave(pass->camera, pass->useMastersSceneData);
    installEffectCullVisitor(pass->camera, pass->collect_lights, pass->effect_scheme);
    _passes.push_back(pass);
}

Buffer *
Compositor::getBuffer(const std::string &name) const
{
    auto it = _buffers.find(name);
    if (it == _buffers.end())
        return 0;
    return it->second.get();
}

Pass *
Compositor::getPass(const std::string &name) const
{
    auto it = std::find_if(_passes.begin(), _passes.end(),
                           [&name](const vsg::ref_ptr<Pass> &p) {
                               return p->name == name;
                           });
    if (it == _passes.end())
        return 0;
    return (*it);
}

} // namespace simgear::compositor
