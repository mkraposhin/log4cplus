// Module:  Log4CPLUS
// File:    objectregistry.cxx
// Created: 3/2003
// Author:  Tad E. Smith
//
//
// Copyright 2003-2017 Tad E. Smith
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <log4cplus/spi/objectregistry.h>
#include <log4cplus/thread/syncprims-pub-impl.h>
#include <log4cplus/thread/threads.h>

#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
#include <catch.hpp>
#endif


namespace log4cplus::spi {


///////////////////////////////////////////////////////////////////////////////
// ObjectRegistryBase ctor and dtor
///////////////////////////////////////////////////////////////////////////////

ObjectRegistryBase::ObjectRegistryBase()
    : locking (true)
{ }


ObjectRegistryBase::~ObjectRegistryBase() = default;



///////////////////////////////////////////////////////////////////////////////
// ObjectRegistryBase public methods
///////////////////////////////////////////////////////////////////////////////

bool
ObjectRegistryBase::exists(const tstring& name) const
{
    std::unique_lock guard {mutex};

    return data.find(name) != data.end();
}


std::vector<tstring>
ObjectRegistryBase::getAllNames() const
{
    std::vector<tstring> tmp;

    {
        std::unique_lock guard {mutex};
        tmp.reserve (data.size ());
        for (auto const & kv : data)
            tmp.emplace_back(kv.first);
    }

    return tmp;
}



///////////////////////////////////////////////////////////////////////////////
// ObjectRegistryBase protected methods
///////////////////////////////////////////////////////////////////////////////

bool
ObjectRegistryBase::putVal(const tstring& name, void* object)
{
    ObjectMap::value_type value(name, object);
    std::pair<ObjectMap::iterator, bool> ret;

    {
        std::unique_lock<std::mutex> guard;
        if (locking)
            guard = std::unique_lock {mutex};

        ret = data.insert(std::move (value));
    }

    if (! ret.second)
        deleteObject( value.second );

    return ret.second;
}


void*
ObjectRegistryBase::getVal(const tstring& name) const
{
    std::unique_lock guard {mutex};

    auto it (data.find (name));
    if (it != data.end ())
        return it->second;
    else
        return nullptr;
}




void
ObjectRegistryBase::clear()
{
    std::unique_lock guard {mutex};

    for (auto const & kv : data)
        deleteObject (kv.second);
}


void
ObjectRegistryBase::_enableLocking (bool enable)
{
    locking = enable;
}


#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
CATCH_TEST_CASE ("ObjectRegistryBase")
{

    class TestObjectRegistry : public ObjectRegistryBase
    {
    public:
        using ObjectRegistryBase::putVal;
        using ObjectRegistryBase::getVal;
        using ObjectRegistryBase::clear;

        virtual void deleteObject(void *object) const
        {
            delete static_cast<std::string *>(object);
        }
    };

    CATCH_SECTION ("put-get")
    {
        TestObjectRegistry reg;
        CATCH_REQUIRE (reg.getVal (LOG4CPLUS_TEXT ("doesnotexist")) == nullptr);
        std::string * const str = new std::string ("test");
        CATCH_REQUIRE (reg.putVal (LOG4CPLUS_TEXT ("a"), str));
        CATCH_REQUIRE (!reg.putVal (LOG4CPLUS_TEXT ("a"), str));
        std::string * str2 = new std::string ("test2");
        CATCH_REQUIRE (reg.putVal (LOG4CPLUS_TEXT ("b"), str2));
        CATCH_REQUIRE (reg.getVal (LOG4CPLUS_TEXT ("a")) == str);
        CATCH_REQUIRE (reg.getVal (LOG4CPLUS_TEXT ("b")) == str2);
    }

}
#endif // defined (LOG4CPLUS_WITH_UNIT_TESTS)

} // namespace log4cplus::spi
