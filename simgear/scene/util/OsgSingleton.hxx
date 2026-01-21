#pragma once

#include <vsg/all.h>

#include <osg/Referenced>

#include <simgear/structure/Singleton.hxx>


namespace simgear {

template <typename RefClass>
class SingletonRefPtr
{
public:
    SingletonRefPtr()
    {
        ptr = new RefClass;
    }
    static RefClass* instance()
    {
        SingletonRefPtr& singleton = boost::details::pool::singleton_default<SingletonRefPtr>::instance();
        return singleton.ptr.get();
    }

private:
    vsg::ref_ptr<RefClass> ptr;
};

template <typename RefClass>
class ReferencedSingleton : public virtual osg::Referenced
{
public:
    static RefClass* instance()
    {
        return SingletonRefPtr<RefClass>::instance();
    }
};

} // namespace simgear
