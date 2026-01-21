#include "SplicingVisitor.hxx"

namespace simgear
{
using namespace osg;

SplicingVisitor::SplicingVisitor()
    : NodeVisitor(NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
    _childStack.push_back(NodeList());
}

void SplicingVisitor::reset()
{
    _childStack.clear();
    NodeVisitor::reset();
}

NodeList SplicingVisitor::traverse(vsg::Node& node)
{
    NodeList result;
    _childStack.push_back(NodeList());
    NodeVisitor::traverse(node);
    result = _childStack.back();
    _childStack.pop_back();
    return result;
}
void SplicingVisitor::apply(vsg::Node& node)
{
    NodeVisitor::traverse(node);
    pushNode(&node);
}

void SplicingVisitor::apply(vsg::Group& node)
{
    if (pushNode(getNewNode(node)))
        return;
    pushResultNode(&node, &node, traverse(node));
}

vsg::Group* SplicingVisitor::pushResultNode( vsg::Group* node,
                                             vsg::Group* newNode,
                                             const osg::NodeList& children )
{
    ref_ptr<Group> result;
    if (node == newNode) {
        result = copyIfNeeded(*node, children);
    } else {
        result = newNode;
        for (NodeList::const_iterator itr = children.begin(), end = children.end();
             itr != end;
             ++itr)
            result->addChild(itr->get());
    }
    _childStack.back().push_back(result);
    recordNewNode(node, result.get());
    return result.get();
}

vsg::Node* SplicingVisitor::pushResultNode(vsg::Node* node, vsg::Node* newNode)
{
    _childStack.back().push_back(newNode);
    recordNewNode(node, newNode);
    return newNode;
}

vsg::Node* SplicingVisitor::pushNode(vsg::Node* node)
{
    if (node)
        _childStack.back().push_back(node);
    return node;
}

vsg::Node* SplicingVisitor::getResult()
{
    NodeList& top = _childStack.at(0);
    if (top.empty()) {
        return 0;
    } else if (top.size() == 1) {
        return top[0].get();
    } else {
        Group* result = new Group;
        for (NodeList::iterator itr = top.begin(), end = top.end();
             itr != end;
             ++itr)
            result->addChild(itr->get());
        return result;
    }
}

vsg::Node* SplicingVisitor::getNewNode(vsg::Node* node)
{
    ref_ptr<Node> tmpPtr(node);
    NodeMap::iterator itr;
    try {
        itr = _visited.find(tmpPtr);
    }
    catch (...) {
        tmpPtr.release();
        throw;
    }
    if (itr == _visited.end())
        return 0;
    else
        return itr->second.get();
}

bool SplicingVisitor::recordNewNode(vsg::Node* oldNode, vsg::Node* newNode)
{
    ref_ptr<Node> oldTmp(oldNode);
    ref_ptr<Node> newTmp(newNode);
    return _visited.insert(std::make_pair(oldTmp, newTmp)).second;
}
}
