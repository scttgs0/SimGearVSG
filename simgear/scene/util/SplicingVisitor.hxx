#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include <vsg/all.h>

#include <osg/NodeVisitor>


namespace simgear {
class SplicingVisitor : public osg::NodeVisitor
{
public:
    META_NodeVisitor(simgear, SplicingVisitor);
    SplicingVisitor();
    virtual ~SplicingVisitor() {}
    virtual void reset();
    osg::NodeList traverse(vsg::Node& node);
    using osg::NodeVisitor::apply;
    virtual void apply(vsg::Node& node);
    virtual void apply(vsg::Group& node);
    template <typename T>
    static T* copyIfNeeded(T& node, const osg::NodeList& children);
    template <typename T>
    static T* copy(T& node, const osg::NodeList& children);
    /**
     * Push the result of processing this node.
     *
     * If the child list is not equal to the node's current children,
     * make a copy of the node. Record the (possibly new) node which
     * should be the returned result if the node is visited again.
     */
    vsg::Group* pushResultNode(vsg::Group* node, vsg::Group* newNode,
                               const osg::NodeList& children);
    /**
     * Push the result of processing this node.
     *
     * Record the (possibly new) node which should be the returned
     * result if the node is visited again.
     */
    vsg::Node* pushResultNode(vsg::Node* node, vsg::Node* newNode);
    /**
     * Push some node onto the list of result nodes.
     */
    vsg::Node* pushNode(vsg::Node* node);
    vsg::Node* getResult();
    vsg::Node* getNewNode(vsg::Node& node)
    {
        return getNewNode(&node);
    }
    vsg::Node* getNewNode(vsg::Node* node);
    bool recordNewNode(vsg::Node* oldNode, vsg::Node* newNode);
    osg::NodeList& getResults() { return _childStack.back(); }

protected:
    std::vector<osg::NodeList> _childStack;
    typedef std::map<vsg::ref_ptr<vsg::Node>, vsg::ref_ptr<vsg::Node>> NodeMap;
    NodeMap _visited;
};

template <typename T>
T* SplicingVisitor::copyIfNeeded(T& node, const osg::NodeList& children)
{
    bool copyNeeded = false;
    if (node.getNumChildren() == children.size()) {
        for (std::size_t i = 0; i < children.size(); ++i)
            if (node.getChild(i) != children[i].get()) {
                copyNeeded = true;
                break;
            }
    } else {
        copyNeeded = true;
    }
    if (copyNeeded)
        return copy(node, children);
    else
        return &node;
}

template <typename T>
T* SplicingVisitor::copy(T& node, const osg::NodeList& children)
{
    T* result = osg::clone(&node, osg::CopyOp::SHALLOW_COPY);
    result->removeChildren(0, result->getNumChildren());
    for (osg::NodeList::const_iterator itr = children.begin(),
                                       end = children.end();
         itr != end;
         ++itr)
        result->addChild(itr->get());
    return result;
}
} // namespace simgear
