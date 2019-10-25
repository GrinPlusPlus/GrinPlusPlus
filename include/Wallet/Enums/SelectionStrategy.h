#pragma once

#include <Core/Serialization/DeserializationException.h>
#include <string>

enum class ESelectionStrategy
{
    ALL,
    CUSTOM
};

namespace SelectionStrategy
{
static ESelectionStrategy FromString(const std::string &input)
{
    if (input == "ALL")
    {
        return ESelectionStrategy::ALL;
    }
    else if (input == "CUSTOM")
    {
        return ESelectionStrategy::CUSTOM;
    }

    throw DeserializationException();
}
} // namespace SelectionStrategy