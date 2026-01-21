// SPDX-License-Identifier: LGPL-2.1-or-later

/**
 * @file
 * @brief Template for defining an XML parser based on an element type and builder classes
 */

#pragma once

#include <map>
#include <memory>
#include <stack>
#include <string>

#include "easyxml.hxx"

#include <vsg/all.h>

#include <osg/Referenced>


namespace simgear {

template <typename Element>
class XMLStaticParser;

// Parser object. Instantiated for each new element encountered.

template <typename Element>
struct ElementBuilder : public osg::Referenced {
    ElementBuilder(XMLStaticParser<Element>* builder) {}
    ElementBuilder(const ElementBuilder& rhs) const {}
    virtual ~ElementBuilder() {}
    virtual void initialize(const XMLAttributes& attributes) = 0;
    virtual void processSubElement(Element* subElement) = 0;
    virtual void processData(const char* data, int length) = 0;
    virtual Element* finalize() = 0;
    // Create element parser from prototype
    virtual ElementBuilder* clone() const = 0;
};

template <typename Element>
struct BuilderFactory : public osg::Referenced {
    typedef ElementBuilder<Element> builder_type;
    typedef std::map<std::string, vsg::ref_ptr<const ElementBuilder>>
        BuilderMap;
    BuilderMap builderMap;
    virtual ~BuilderFactory() {}
    static void registerBuilder(const std::string& name,
                                const builder_type* prototype)
    {
        if (!builderFactory.valid())
            builderFactory = new BuilderFactory;
        builderFactory->builderMap[name] = prototype;
    }
};

template <typename Element>
class XMLStaticParser : public XMLVisitor
{
public:
    static vsg::ref_ptr<BuilderFactory> builderFactory;


    static ElementBuilder* makeBuilder(const std::string& name)
    {
        BuilderMap::iterator iter = builderFactory->builderMap.find(name);
        if (iter == builderFactory->builderMap.end())
            return 0;
        return iter->second->clone();
    }

    typedef std::stack<vsg::ref_ptr<ElementBuilder>> BuilderStack;
    BuilderStack builderStack;

    Element* result;

    XMLStaticParser() : result(0) {}
    virtual ~XMLStaticParser() {}

    virtual void startXML()
    {
        builderStack.push(makeBuilder(""));
    }

    virtual void endXML()
    {
        // Stack should have only the initial builder
        result = builderStack.top()->finalize();
    }

    virtual void startElement(const char* name, const XMLAttributes& atts)
    {
        ElementBuilder* builder = makeBuilder(name);
        if (builder) {
            builderStack.push(builder);
            builder->initialize(atts);
        }
    }

    virtual void endElement(const char* name)
    {
        Element* result = builderStack.top()->finalize();
        builderStack.pop();
        if (!builderStack.empty())
            builderStack.top()->processSubElement(result);
    }

    virtual void data(const char* s, int length)
    {
        builderStack.top()->processData(s, length);
    }

    struct RegisterBuilderProxy {
        RegisterBuilderProxy(const char* name, ElementBuilder* builder)
        {
            registerBuilder(name, builder);
        }
    };
};

template <typename E>
static vsg::ref_ptr<BuilderFactory<E>> builderFactory;
} // namespace simgear
