// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2007 Tim Moore <timoore@redhat.com>
// SPDX-FileCopyrightText: 2008 Till Busch <buti@bux.at>

#include <simgear_config.h>

#include <algorithm>
#include <optional>
//yuck
#include <cstring>
#include <cassert>

#include <osg/Version>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Switch>
#include <osgDB/FileNameUtils>

#include <simgear/compiler.h>
#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>

#include <simgear/scene/util/FindGroupVisitor.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/structure/exception.hxx>

#include "modellib.hxx"
#include "SGReaderWriterXML.hxx"

#include "animation.hxx"
#include "particles.hxx"
#include "model.hxx"
#include "SGLight.hxx"
#include "SGText.hxx"

using namespace std;
using namespace simgear;
using namespace osg;

static std::tuple<int, vsg::Node *>
sgLoad3DModel_internal(const SGPath& path,
                       const osgDB::Options* options,
                       SGPropertyNode *overlay = 0);


SGReaderWriterXML::SGReaderWriterXML()
{
    supportsExtension("xml", "SimGear xml database format");
}

SGReaderWriterXML::~SGReaderWriterXML()
{
}

const char* SGReaderWriterXML::className() const
{
    return "XML database reader";
}

osgDB::ReaderWriter::ReadResult
SGReaderWriterXML::readNode(const std::string& name,
                            const osgDB::Options* options) const
{
    std::string fileName = osgDB::findDataFile(name, options);
    simgear::ErrorReportContext ec{"model-xml", fileName};

    vsg::Node *result=0;
    try {
        SGPath p = SGModelLib::findDataFile(fileName);
        if (!p.exists()) {
          return ReadResult::FILE_NOT_FOUND;
        }

        int num_anims;
        std::tie(num_anims, result) = sgLoad3DModel_internal(p, options);
    } catch (const sg_exception &t) {
        SG_LOG(SG_IO, SG_DEV_ALERT, "Failed to load model: " << t.getFormattedMessage()
          << "\n\tfrom:" << fileName);
        result=new vsg::Node;
    }
    if (result)
        return result;
    else
        return ReadResult::FILE_NOT_HANDLED;
}

class SGSwitchUpdateCallback : public osg::NodeCallback
{
public:
    SGSwitchUpdateCallback(SGCondition* condition) :
            mCondition(condition) {}
    virtual void operator()(vsg::Node* node, osg::NodeVisitor* nv) {
        assert(dynamic_cast<osg::Switch*>(node));
        osg::Switch* s = static_cast<osg::Switch*>(node);

        if (mCondition && mCondition->test()) {
            s->setAllChildrenOn();
            // note, callback is responsible for scenegraph traversal so
            // should always include call traverse(node,nv) to ensure
            // that the rest of cullbacks and the scene graph are traversed.
            traverse(node, nv);
        } else
            s->setAllChildrenOff();
    }

private:
    SGSharedPtr<SGCondition> mCondition;
};


// Little helper class that holds an extra reference to a
// loaded 3d model.
// Since we clone all structural nodes from our 3d models,
// the database pager will only see one single reference to
// top node of the model and expire it relatively fast.
// We attach that extra reference to every model cloned from
// a base model in the pager. When that cloned model is deleted
// this extra reference is deleted too. So if there are no
// cloned models left the model will expire.
namespace {
class SGDatabaseReference : public osg::Observer
{
public:
    SGDatabaseReference(osg::Referenced* referenced) :
        mReferenced(referenced)
    { }
    virtual void objectDeleted(void*)
    {
        mReferenced = 0;
    }
private:
    vsg::ref_ptr<osg::Referenced> mReferenced;
};

}

namespace simgear {
class SetNodeMaskVisitor : public osg::NodeVisitor {
public:
    SetNodeMaskVisitor(vsg::Node::NodeMask nms, vsg::Node::NodeMask nmc) :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), nodeMaskSet(nms), nodeMaskClear(nmc)
    {}
    virtual void apply(osg::Geode& node) {
        node.setNodeMask((node.getNodeMask() | nodeMaskSet) & ~nodeMaskClear);
        traverse(node);
    }
private:
    vsg::Node::NodeMask nodeMaskSet;
    vsg::Node::NodeMask nodeMaskClear;
};

}

namespace {
    class ExcludeInPreview
    {
    public:
        bool operator()(SGPropertyNode* aNode) const
        {
            string typeString(aNode->getStringValue("type"));
            // exclude these so we don't show yellow outlines in preview mode
            return (typeString == "pick") || (typeString == "knob") || (typeString == "slider") || (typeString == "touch");
        }
    };

    bool removeNamedNode(vsg::Group* aGroup, const std::string& aName)
    {
        int nKids = aGroup->getNumChildren();
        for (int i = 0; i < nKids; i++) {
            vsg::Node* child = aGroup->getChild(i);
            if (child->getName() == aName) {
                aGroup->removeChild(child);
                return true;
            }
        }

        for (int i = 0; i < nKids; i++) {
            vsg::Group* childGroup = aGroup->getChild(i)->asGroup();
            if (!childGroup)
                continue;

            if (removeNamedNode(childGroup, aName))
                return true;
        } // of child groups traversal

        return false;
    }
}

struct DumpSGPropertyNode {
    SGPropertyNode*     _node;
    const std::string&  _indent;

    DumpSGPropertyNode(SGPropertyNode* node, const std::string& indent="")
    :
    _node(node),
    _indent(indent)
    {}

    friend std::ostream& operator << (std::ostream& out, const DumpSGPropertyNode& dump)
    {
        if (!dump._node)    return out;
        out << dump._indent << dump._node->getDisplayName() << "=" << dump._node->getStringValue() << "\n";
        for (int i=0; i<dump._node->nChildren(); ++i) {
            SGPropertyNode* child = dump._node->getChild(i);
            out << DumpSGPropertyNode(child, dump._indent + "    ");
        }
        return out;
    }
};

struct DumpOsgNode {
    vsg::Node*          _node;
    const std::string&  _indent;

    DumpOsgNode(vsg::Node* node, const std::string& indent="")
    :
    _node(node),
    _indent(indent)
    {}

    friend std::ostream& operator << (std::ostream& out, const DumpOsgNode& dump)
    {
        if (!dump._node)    return out;
        out << dump._indent << dump._node->getName() << "\n";
        vsg::Group* group = dynamic_cast<vsg::Group*>(dump._node);
        if (group) {
            for (unsigned i=0; i<group->getNumChildren(); ++i) {
                out << DumpOsgNode(group->getChild(i), dump._indent + "    ");
            }
        }
        return out;
    }
};

/* Finds all names recursively in an vsg::Node. */
struct OSGNodeGetNames
{
    std::vector<std::string>    names;

    OSGNodeGetNames(vsg::Node* node=NULL)
    {
        add(node);
    }

    void add(vsg::Node* node)
    {
        if (!node)  return;
        const std::string&  name = node->getName();
        if (name != "") names.push_back(name);

        vsg::Group* group = dynamic_cast<vsg::Group*>(node);
        if (group) {
            for (unsigned i=0; i<group->getNumChildren(); ++i) {
                add(group->getChild(i));
            }
        }
    }
};


/* For each existing animation in <props>, we add a tooltip showing information
on the properties that the animation depends on.

At runtime, the tooltips only show if sim/animation-tooltips is true.

The dummy animations will show up as yellow (like clickable items) if the user
presses Ctrl-C, even if tooltips aren't showing because sim/animation-tooltips
is false. */
void addTooltipAnimations(const SGPath& path, SGPropertyNode_ptr props, vsg::ref_ptr<vsg::Node> model, int autoTooltipsMasterMax)
{
    OSGNodeGetNames model_names;
    if (0) {
        /* Include all names in the .asi file, in an attempt to make
        tooltips activate for more than just the needle in instrument dials
        for example. This doesn't seem to help. */
        model_names.add(model);
        if (!model_names.names.empty()) {
            /* First name will be filename of .ac file. */
            model_names.names.erase(model_names.names.begin());
        }
    }

    /* For each animation add an extra animation with type=pick containing
    set-tooltip. We use the object-name as the tooltip-id, and we use the
    animation's objectname and property name/value(s) in the tooltip label. */
    PropertyList animations = props->getChildren("animation");
    SG_LOG(SG_INPUT, SG_BULK, "animations.size()=" << animations.size()
            << " path=" << path
            << " props=" << (props ? props->getPath() : "")
            );

    /* We want to add to any existing tooltip for a particular object-name
    (e.g. joysticks might have separate animations for elevator and aileron
    control properties), so we keep track of the ones we've created in this
    map. */
    std::map<std::string, SGPropertyNode*>  object_name_to_animation_node;

    /* Keep track of the total number of animations, so we can respect
    autoTooltipsMasterMax. */
    static int  num_new_animations = 0;

    for (unsigned i = 0; i < animations.size(); ++i) {

        SGPropertyNode* animation = animations[i];

        if (animation->getStringValue("type") == "pick") {
            /* There appear to be many of these, and we end up consuming
            GB's of memory if we install a tooltip for each one, so ignore.
            */
            continue;
        }

        PropertyList    properties = animation->getChildren("property");
        if (properties.empty()) {
            /* We can't really show anything useful in tooltip. */
            continue;
        }

        PropertyList objectnames(animation->getChildren("object-name"));
        if (objectnames.empty()) {
            continue;
        }

        /* Make a unique tooltip-id. */
        std::string tooltip_id = "auto-tooltip-";
        {
            if (autoTooltipsMasterMax > 0 && num_new_animations > autoTooltipsMasterMax) {
                continue;
            }
            num_new_animations += 1;
            std::ostringstream  s;
            s << num_new_animations;
            tooltip_id += s.str();
        }

        /* If we reach here, we create a new dummy animation with type="pick"
        that will implement a tooltip with information about <animation>. */

        SGPropertyNode* new_animation = nullptr;
        std::string     objectname_for_label;

        /* Use <animation>'s object-names so that our tooltip appears whenever
        the user hovers over <animation>'s objects. */
        for (unsigned i=0; i<objectnames.size(); ++i) {
            std::string objectname = objectnames[i]->getStringValue();
            SGPropertyNode*&    n = object_name_to_animation_node[objectname];
            if (n) {
                /* We're adding to an existing animation so might as well
                correct num_new_animations. */
                num_new_animations -= 1;
            }
            else {
                n = props->addChild("animation");
                n->addChild("object-name")->setStringValue(objectname);
                n->addChild("type")->setValue("pick");
            }
            new_animation = n;
            objectname_for_label = objectname;
        }

        /* We could be adding to previously-created animation or setting up a
        new animation. So we use getChild() with create=true to populate. */
        SGPropertyNode* hovered_binding = new_animation
                ->getChild("hovered", 0 /*index*/, true /*create*/)
                ->getChild("binding", 0 /*index*/, true /*create*/)
                ;
        hovered_binding
                ->getChild("command", 0 /*index*/, true /*create*/)
                ->setValue("set-tooltip")
                ;
        hovered_binding
                ->getChild("condition", 0 /*index*/, true /*create*/)
                ->getChild("property", 0 /*index*/, true /*create*/)
                ->setValue("sim/animation-tooltips")
                ;
        if (!hovered_binding->getChild("tooltip-id")) {
            hovered_binding
                    ->getChild("tooltip-id", 0 /*index*/, true /*create*/)
                    ->setStringValue(tooltip_id)
                    ;
        }

        std::string label = hovered_binding->getStringValue("label");
        if (label == "") {
            label += objectname_for_label + ":";
        }

        /* Build up printf-style label string showing the property values
        that <animation> depend on. */
        for (size_t i=0; i<properties.size(); ++i) {
            label += " ";
            SGPropertyNode* p = properties[i];
            /* Using an alias here (rather then just copying the path) ensures
            things work if <p> is itself an alias. */
            hovered_binding->addChild("property")->alias(p, false);
            if (p->isAlias()) {
                /* We only get things like "/rpm[0]" here, rather than the full
                path of the property to which p points. */
                label += p->getAliasTarget()->getPath(true /*simplify*/);
            }
            else {
                label += p->getStringValue();
            }
            label += "=%s";
        }
        hovered_binding->setStringValue("label", label);

        SG_LOG(SG_INPUT, SG_BULK,
                "have added/updated auto-tooltip."
                << " num_new_animations=" << num_new_animations
                << " new_animation=" << new_animation << ":\n"
                << DumpSGPropertyNode(new_animation, "    ")
                );
    }

    /* This is very verbose, so disabled even at level SG_BULK. */
    if (0) {
        SG_LOG(SG_INPUT, SG_BULK, "props for path=" << path << ":\n"
                << DumpSGPropertyNode(props, "    ")
                );
    }

    SG_LOG(SG_INPUT, SG_DEBUG, "auto-tooltips: num_new_animations=" << num_new_animations);
}

/*
 * Search a parent group by name and attach a child node to it
 *
 * @param group     Root group in which to search for the desired parent
 * @param child     New node to attach to to the parent
 * @param config    Searched name is set by property "attach-to" under this node
 * @param path      File path for error reporting
 *
 * If "attach-to" is not defined in `config`, silently attach `child` to `group`.
 * If it is defined, but no such parent is found, report an error and attach to `group` anyways.
 */
static void findAndAttach(vsg::Group* group, vsg::Node* child, const SGPropertyNode_ptr config, const SGPath& path)
{
    const SGPropertyNode_ptr attach = config->getNode("attach-to", false);
    std::string name;
    std::string err;

    if (attach) {
        name = attach->getStringValue();

        if (name.empty())
            err = "Ignoring empty <attach-to> tag";
    }

    if (!name.empty()) {
        simgear::FindGroupVisitor visitor(name);
        group->accept(visitor);

        if (visitor.getGroup()) {
            if (visitor.foundDuplicates())
                err = "Found several groups named '" + name + "'";
            // In case of duplicates, this will be the first group found
            group = visitor.getGroup();
        } else {
            err = "Could not find group '" + name + "'";
        }
    }

    // Report error
    if (!err.empty()) {
        std::string childName = child->getName();
        if (childName.empty())
            childName = "<unnamed>";

        simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::XMLModelLoad,
                               err + " to attach '" + childName + "'", path);
    }

    // even in case of failure, attach to the root group to have something
    group->addChild(child);
}

static std::tuple<int, vsg::Node *>
sgLoad3DModel_internal(const SGPath& path,
                       const osgDB::Options* dbOptions,
                       SGPropertyNode *overlay)
{
    SGPath modelpath(path);
    SGPath texturepath(path);

    vsg::ref_ptr<SGReaderWriterOptions> options;
    options = SGReaderWriterOptions::copyOrCreate(dbOptions);


    SGPath modelDir(modelpath.dir());
    int animationcount = 0;

    SGSharedPtr<SGPropertyNode> prop_root = options->getPropertyNode();
    if (!prop_root.valid())
        prop_root = new SGPropertyNode;
    // The model data appear to be only used in the topmost model
    vsg::ref_ptr<SGModelData> data = options->getModelData();
    options->setModelData(0);

    // remember the current value of the vertex order setting
    // because an included <model> may change this.
    bool currentVertexOrderXYZ = options->getVertexOrderXYZ();

    vsg::ref_ptr<vsg::Node> model;
    vsg::ref_ptr<vsg::Group> group;
    SGPropertyNode_ptr props = new SGPropertyNode;
    bool previewMode = (dbOptions->getPluginStringData("SimGear::PREVIEW") == "ON");

    // Check for an XML wrapper
    if (modelpath.extension() == "xml") {
       try {
           readProperties(modelpath, props);
        } catch (const sg_exception &t) {
            simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::XMLModelLoad,
                                   "Failed to load model XML:" + t.getFormattedMessage(), t.getLocation());
            SG_LOG(SG_IO, SG_DEV_ALERT, "Failed to load xml: "
                   << t.getFormattedMessage());
            throw;
        }

        if (overlay)
            copyProperties(overlay, props);

        if (options->getAutoTooltipsMaster()) {
            addTooltipAnimations(path, props, model, options->getAutoTooltipsMasterMax());
        }

        if (previewMode && props->hasChild("nopreview")) {
            return std::make_tuple(0, (vsg::Node *) NULL);
        }

        if (props->hasChild("defaults")) {
            SGPropertyNode_ptr defaultsNode = props->getNode("defaults");
            if (defaultsNode->hasChild("axis-animation-vertex-order-xyz"))
                options->setVertexOrderXYZ(true);
            if (defaultsNode->hasChild("axis-animation-vertex-order-x"))
                options->setVertexOrderXYZ(false);
        }
        if (props->hasValue("/path")) {
            string modelPathStr = props->getStringValue("/path");
            modelpath = SGModelLib::findDataFile(modelPathStr, NULL, modelDir);
            if (modelpath.isNull()) {
                simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::ThreeDModelLoad,
                                       "Model not found:" + modelPathStr, path);
                throw sg_io_exception("Model file not found: '" + modelPathStr + "'",
                                      path, {}, false);
            }

            if (props->hasValue("/texture-path")) {
                string texturePathStr = props->getStringValue("/texture-path");
                if (!texturePathStr.empty())
                {
                    texturepath = SGModelLib::findDataFile(texturePathStr, NULL, modelDir);
                    if (texturepath.isNull()) {
                        simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::LoadingTexture,
                                               "Texture file not found:" + texturePathStr, path);
                        throw sg_io_exception("Texture file not found: '" + texturePathStr + "'",
                                path);
                    }
                }
            }
        } else {
            model = new vsg::Node;
        }

        SGPropertyNode *mp = props->getNode("multiplay");
        if (mp && prop_root && prop_root->getParent())
            copyProperties(mp, prop_root);
    } else {
        // model without wrapper
    }

    // Assume that textures are in
    // the same location as the XML file.
    if (!model) {
        if (!texturepath.extension().empty())
            texturepath = texturepath.dir();

        options->setDatabasePath(texturepath.utf8Str());
        options->setPluginStringData("filePath", modelpath.utf8Str());

        osgDB::ReaderWriter::ReadResult modelResult;
        modelResult = osgDB::readRefNodeFile(modelpath.utf8Str(), options.get());

        if (!modelResult.validNode()) {
            simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::XMLModelLoad,
                                   "Failed to load 3D model:" + modelResult.message(), modelpath);
            throw sg_io_exception("Failed to load 3D model:" + modelResult.message(),
                                  modelpath, {}, false);
        }

        model = copyModel(modelResult.getNode());
        // Add an extra reference to the model stored in the database.
        // That is to avoid expiring the object from the cache even if
        // it is still in use. Note that the object cache will think
        // that a model is unused if the reference count is 1. If we
        // clone all structural nodes here we need that extra
        // reference to the original object
        SGDatabaseReference* databaseReference;
        // REVIEW: Memory Leak - 4,352 bytes in 272 blocks are definitely lost
        databaseReference = new SGDatabaseReference(modelResult.getNode());
        model->addObserver(databaseReference);

        // Update liveries
        TextureUpdateVisitor liveryUpdate(options->getDatabasePathList());
        model->accept(liveryUpdate);

        // Copy the userdata fields, still sharing the boundingvolumes,
        // but introducing new data for velocities.
        UserDataCopyVisitor userDataCopyVisitor;
        model->accept(userDataCopyVisitor);

        SetNodeMaskVisitor setNodeMaskVisitor(0, simgear::MODELLIGHT_BIT);
        model->accept(setNodeMaskVisitor);
    }
    model->setName(modelpath.utf8Str());

    bool needTransform=false;
    // Set up the alignment node if needed
    SGPropertyNode *offsets = props->getNode("offsets", false);
    if (offsets) {
        needTransform=true;
        osg::MatrixTransform *alignmainmodel = new osg::MatrixTransform;
        alignmainmodel->setDataVariance(vsg::Object::STATIC);
        vsg::mat4 res_matrix;
        res_matrix.makeRotate(
            offsets->getFloatValue("pitch-deg", 0.0)*SG_DEGREES_TO_RADIANS,
            vsg::vec3(0, 1, 0),
            offsets->getFloatValue("roll-deg", 0.0)*SG_DEGREES_TO_RADIANS,
            vsg::vec3(1, 0, 0),
            offsets->getFloatValue("heading-deg", 0.0)*SG_DEGREES_TO_RADIANS,
            vsg::vec3(0, 0, 1));

        vsg::mat4 tmat;
        tmat.makeTranslate(offsets->getFloatValue("x-m", 0.0),
                           offsets->getFloatValue("y-m", 0.0),
                           offsets->getFloatValue("z-m", 0.0));
        alignmainmodel->setMatrix(res_matrix*tmat);
        group = alignmainmodel;
    }
    if (!group) {
        group = new vsg::Group;
    }
    group->addChild(model.get());

    // Load sub-models
    vector<SGPropertyNode_ptr> model_nodes = props->getChildren("model");
    for (unsigned i = 0; i < model_nodes.size(); i++) {
        SGPropertyNode_ptr sub_props = model_nodes[i];

        SGPath submodelpath;
        vsg::ref_ptr<vsg::Node> submodel;

        string subPathStr = sub_props->getStringValue("path");
        SGPath submodelPath = SGModelLib::findDataFile(subPathStr,
          NULL, modelDir);

        if (submodelPath.isNull()) {
          SG_LOG(SG_IO, SG_DEV_ALERT, "Failed to load file: \"" << subPathStr << "\"");
          simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::XMLModelLoad,
                                 "Couldn't find file for submodel '" + sub_props->getStringValue("name") + "': " + subPathStr,
                                 sg_location(sub_props));
          continue;
        }

        if(sub_props->hasChild("usage")){ /* We don't want load this file and its content now */
            bool isInterior = (std::string(sub_props->getStringValue("usage")) == "interior");
            bool isAI = (std::string(prop_root->getStringValue("type")) == "AI");
            if(isInterior && isAI){
                 props->addChild("interior-path")->setStringValue(submodelPath.utf8Str());
                 continue;
            }
        }

        simgear::ErrorReportContext("submodel", submodelPath.utf8Str());

        try {
            int num_anims;
            std::tie(num_anims, submodel) = sgLoad3DModel_internal(submodelPath, options.get(),
                                              sub_props->getNode("overlay"));
            animationcount += num_anims;
        } catch (const sg_exception &t) {
            SG_LOG(SG_IO, SG_DEV_ALERT, "Failed to load submodel: " << t.getFormattedMessage()
              << "\n\tfrom:" << t.getOrigin());
            continue;
        }

        if (!submodel)
            continue;

        vsg::ref_ptr<vsg::Node> submodel_final = submodel;
        SGPropertyNode *offs = sub_props->getNode("offsets", false);
        if (offs) {
            vsg::mat4 res_matrix;
            vsg::ref_ptr<osg::MatrixTransform> align = new osg::MatrixTransform;
            align->setDataVariance(vsg::Object::STATIC);
            res_matrix.makeIdentity();
            res_matrix.makeRotate(
                offs->getDoubleValue("pitch-deg", 0.0)*SG_DEGREES_TO_RADIANS,
                vsg::vec3(0, 1, 0),
                offs->getDoubleValue("roll-deg", 0.0)*SG_DEGREES_TO_RADIANS,
                vsg::vec3(1, 0, 0),
                offs->getDoubleValue("heading-deg", 0.0)*SG_DEGREES_TO_RADIANS,
                vsg::vec3(0, 0, 1));

            vsg::mat4 tmat;
            tmat.makeIdentity();
            tmat.makeTranslate(offs->getDoubleValue("x-m", 0),
                               offs->getDoubleValue("y-m", 0),
                               offs->getDoubleValue("z-m", 0));
            align->setMatrix(res_matrix*tmat);
            align->addChild(submodel.get());
            submodel_final = align;
        }
        submodel_final->setName(sub_props->getStringValue("name", ""));

        SGPropertyNode *cond = sub_props->getNode("condition", false);
        if (cond) {
            vsg::ref_ptr<osg::Switch> sw = new osg::Switch;
            sw->setUpdateCallback(new SGSwitchUpdateCallback(sgReadCondition(prop_root, cond)));
            findAndAttach(group, sw.get(), sub_props, path);
            sw->addChild(submodel_final.get());
            sw->setName("submodel condition switch");
        } else {
            findAndAttach(group, submodel_final.get(), sub_props, path);
        }
    } // end of submodel loading

    auto particlesManager = ParticlesGlobalManager::instance();
    if (particlesManager->isEnabled()) { //dbOptions->getPluginStringData("SimGear::PARTICLESYSTEM") != "OFF") {
        std::vector<SGPropertyNode_ptr> particle_nodes;
        particle_nodes = props->getChildren("particlesystem");
        for (unsigned i = 0; i < particle_nodes.size(); ++i) {
            SG_LOG(SG_PARTICLES, SG_DEBUG, "Reading in particle " << i << " from file: " << path);
            vsg::ref_ptr<SGReaderWriterOptions> options2;
            options2 = new SGReaderWriterOptions(*options);
            if (i==0) {
                if (!texturepath.extension().empty())
                    texturepath = texturepath.dir();

                options2->setDatabasePath(texturepath.utf8Str());
            }
            vsg::ref_ptr<vsg::Node> particle;
            particle = particlesManager->appendParticles(particle_nodes[i],
                                                         prop_root,
                                                         options2.get());
            findAndAttach(group, particle, particle_nodes[i], path);
        }
    }

    std::vector<SGPropertyNode_ptr> text_nodes;
    text_nodes = props->getChildren("text");
    for (unsigned i = 0; i < text_nodes.size(); ++i) {
        vsg::ref_ptr<vsg::Node> text;
        text = SGText::appendText(text_nodes[i], prop_root, options.get());
        findAndAttach(group, text, text_nodes[i], path);
    }

    std::vector<SGPropertyNode_ptr> light_nodes;
    light_nodes = props->getChildren("light");
    for (unsigned i = 0; i < light_nodes.size(); ++i) {
        vsg::ref_ptr<vsg::Node> light;
        light = SGLight::appendLight(light_nodes[i], prop_root, false /* legacy mode */);
        findAndAttach(group, light, light_nodes[i], path);
    }

    PropertyList effect_nodes = props->getChildren("effect");
    PropertyList animation_nodes = props->getChildren("animation");

    if (previewMode) {
        PropertyList::iterator it;
        it = std::remove_if(animation_nodes.begin(), animation_nodes.end(), ExcludeInPreview());
        animation_nodes.erase(it, animation_nodes.end());
    }

    {
        ref_ptr<Node> modelWithEffects = instantiateEffects(group.get(), effect_nodes, options.get(), path);
        group = static_cast<Group*>(modelWithEffects.get());
    }

    simgear::SGTransientModelData modelData(group.get(), prop_root, options.get(), path.utf8Str());

    for (unsigned i = 0; i < animation_nodes.size(); ++i) {
        if (previewMode && animation_nodes[i]->hasChild("nopreview")) {
            PropertyList names(animation_nodes[i]->getChildren("object-name"));
            for (unsigned int n=0; n<names.size(); ++n) {
                removeNamedNode(group, names[n]->getStringValue());
            } // of object-names in the animation
            continue;
        }

        try {
            /*
             * Setup the model data for the node currently being animated.
             */
            modelData.LoadAnimationValuesForElement(animation_nodes[i], i);

            /// OSGFIXME: duh, why not only model?????
            SGAnimation::animate(modelData);

            // note from James: we used to re-throw these errors, and they
            // were caught one level up as an SG_DEV_ALERT log message and
            // loading an empty vsg::Node for the entire model.
            // Choosing instead to trap them there, since a single failed animation
            // isn't necessarily a reason to abandon the model load.
        } catch (sg_exception& e) {
            simgear::reportFailure(simgear::LoadFailure::Misconfigured, simgear::ErrorCode::XMLModelLoad,
                                   "Couldn't load animation " + animation_nodes[i]->getNameString() + ":" + e.getFormattedMessage(),
                                   modelpath);
        } catch (std::exception& e) { // needed because props throws std::runtime_error sometimes
            simgear::reportFailure(simgear::LoadFailure::Misconfigured, simgear::ErrorCode::XMLModelLoad,
                                   "Couldn't load animation " + animation_nodes[i]->getNameString() + ":" + e.what(),
                                   modelpath);
        }
    }

    animationcount += animation_nodes.size();

    // restore the vertex order in case a submodel changed it.
    options->setVertexOrderXYZ(currentVertexOrderXYZ);

    if (!needTransform && group->getNumChildren() < 2) {
        model = group->getChild(0);
        group->removeChild(model.get());
        if (data.valid())
            data->modelLoaded(modelpath.utf8Str(), props, model.get());
        return std::make_tuple(animationcount, model.release());
    }
    if (data.valid())
        data->modelLoaded(modelpath.utf8Str(), props, group.get());
    if (props->hasChild("debug-outfile")) {
        std::string outputfile = props->getStringValue("debug-outfile",
                                 "debug-model.osg");
        osgDB::writeNodeFile(*group, outputfile);
    }

    SG_LOG(SG_GENERAL, SG_DEBUG, "Model " << path << " animation count: " << animationcount);

    return std::make_tuple(animationcount, group.release());
}
