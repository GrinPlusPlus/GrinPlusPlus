#pragma once

#include <Core/Util/JsonUtil.h>
#include <iostream>

class JsonComparator
{
public:
    struct Options
    {

    };

    JsonComparator(const Options& options = {})
        : m_options(options) { }

    // TODO: Finish this
    bool Compare(const Json::Value& expected, const Json::Value& actual)
    {
        bool result = true;

        if (expected.isObject())
        {
            if (!CompareObjects(expected, actual))
            {
                result = false;
            }
        }
        else if (expected.isArray())
        {
            if (!CompareArrays(expected, actual))
            {
                result = false;
            }
        }
        else if (expected.compare(actual) != 0)
        {
            result = false;
        }

        return result;
    }

private:
    bool CompareObjects(const Json::Value& expected, const Json::Value& actual)
    {
        bool result = true;

        for (const std::string& member : expected.getMemberNames())
        {
            if (!actual.isMember(member))
            {
                std::cout << "Expected member '" << member << "', but not found." << std::endl;
                result = false;
            }
            else if (!Compare(expected[member], actual[member]))
            {
                std::cout << "Expected value " << expected.asString() << " for member" << member 
                    << ", but was " << actual.asString() << std::endl;
                result = false;
            }
        }

        return result;
    }

    bool CompareArrays(const Json::Value& expected, const Json::Value& actual)
    {
        return expected.compare(actual) == 0;
    }

    Options m_options;
};