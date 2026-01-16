// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2001 David Megginson <david@megginson.com>

/**
 * @file
 * @brief Interface definition for encapsulated commands
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>
#include "SGBinding.hxx"
#include "SGExpression.hxx"

#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

SGAbstractBinding::SGAbstractBinding()
    : _arg(new SGPropertyNode)
{
}

void SGAbstractBinding::clear()
{
    _arg.clear();
}

void SGAbstractBinding::fire() const
{
    if (test()) {
        innerFire();
    }
}

void SGAbstractBinding::fire(SGPropertyNode* params) const
{
    if (test()) {
        if (params != nullptr) {
            copyProperties(params, _arg);
        }

        innerFire();
    }
}

void SGAbstractBinding::fire(double offset, double max) const
{
    if (test()) {
        _arg->setDoubleValue("offset", offset / max);
        innerFire();
    }
}

void SGAbstractBinding::fire(double setting) const
{
    if (test()) {
        // A value is automatically added to
        // the args
        if (!_setting) { // save the setting node for efficiency
            _setting = _arg->getChild("setting", 0, true);
        }
        _setting->setDoubleValue(setting);
        innerFire();
    }
}

///////////////////////////////////////////////////////////////////////////////

SGBinding::SGBinding() = default;

SGBinding::SGBinding(const std::string& commandName)
{
    _command_name = commandName;
}

SGBinding::SGBinding(const SGPropertyNode* node, SGPropertyNode* root)
{
  read(node, root);
}

void
SGBinding::clear()
{
  SGAbstractBinding::clear();
  _root.clear();
  _setting.clear();
}

/**
 * @brief Create a binding object from node
 * node: binding configuration
 * root: property tree root
 */
void SGBinding::read(const SGPropertyNode* node, SGPropertyNode* root)
{
    const SGPropertyNode* conditionNode = node->getChild("condition");
    const SGPropertyNode* expressionNode = node->getChild("expression");
    const SGPropertyNode* target = node->getChild("property");
    _debug = node->getBoolValue("debug", false);

    if (conditionNode != 0)
        setCondition(sgReadCondition(root, conditionNode));


    _command_name = node->getStringValue("command", "");
    if (_command_name.empty() && expressionNode == nullptr) {
        SG_LOG(SG_INPUT, SG_WARN, "Neither command nor expression supplied for binding { " << node->getPath() << " }.");
    }

    _arg = const_cast<SGPropertyNode*>(node);
    _root = const_cast<SGPropertyNode*>(root);
    _setting.clear();

    // if we have no command, look for expression and target property
    if (expressionNode && expressionNode->nChildren() && target) {
        _target_property = _root->getNode(target->getStringValue(), true);
        // input value is stored in 'setting'
        _setting = _arg->getChild("setting", 0, true);

        if (_debug) {
            SG_LOG(SG_INPUT, SG_MANDATORY_INFO, "Reading expression for binding " << node->getPath());
            SG_LOG(SG_INPUT, SG_MANDATORY_INFO, "Input from " << _setting->getPath());
            SG_LOG(SG_INPUT, SG_MANDATORY_INFO, "Output to " << _target_property->getPath());
        }
        /*
        Pass the setting node as property tree root to expression.
        Absolute property paths in <expression> XML will work as usual
        An empty path or '.' will refer to the 'setting' node, i.e. the binding input.
        */
        _expression = SGReadDoubleExpression(_setting, expressionNode->getChild(0));
        if (!_expression && _debug)
            SG_LOG(SG_INPUT, SG_MANDATORY_INFO, "FAILED");
    }

}

void
SGBinding::innerFire () const
{
    auto cmd = SGCommandMgr::instance()->getCommand(_command_name);
    // first try command
    if (cmd) {
        try {
            if (!(*cmd)(_arg, _root)) {
                SG_LOG(SG_INPUT, SG_ALERT, "Failed to execute command " << _command_name);
            }
        } catch (sg_exception& e) {
            SG_LOG(SG_GENERAL, SG_ALERT, "command '" << _command_name << "' failed with exception\n"
                                                    << "\tmessage:" << e.getMessage() << " (from " << e.getOrigin() << ")");
        }
    }
    // otherwise try expression
    else {
        if (_expression) {
            double result = _expression->getDoubleValue();
            if (_debug) {
                SG_LOG(SG_INPUT, SG_MANDATORY_INFO, "Expression result {" << _arg->getPath() << "}:" << result);
            }
            if (_target_property)
                _target_property->setDoubleValue(result);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void fireBindingList(const SGBindingList& aBindings, SGPropertyNode* params)
{
    for (auto b : aBindings) {
        b->fire(params);
    }
}

void fireBindingList(const std::vector<SGBinding_ptr>& aBindings, SGPropertyNode* params)
{
    for (auto b : aBindings) {
        b->fire(params);
    }
}

void fireBindingListWithOffset(const SGBindingList& aBindings, double offset, double max)
{
    for (auto b : aBindings) {
        b->fire(offset, max);
    }
}

SGBindingList readBindingList(const simgear::PropertyList& aNodes, SGPropertyNode* aRoot)
{
    SGBindingList result;
    for (auto node : aNodes) {
        result.push_back(new SGBinding(node, aRoot));
    }

    return result;
}

void clearBindingList(const SGBindingList& aBindings)
{
    for (auto b : aBindings) {
        b->clear();
    }
}

bool anyBindingEnabled(const SGBindingList& aBindings)
{
    if (aBindings.empty()) {
        return false;
    }

    for (auto b : aBindings) {
        if (b->test()) {
            return true;
        }
    }

    return false;
}
