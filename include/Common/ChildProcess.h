#pragma once

#include <string>
#include <vector>
#include <memory>
#include <array>

// Forward Declarations
namespace reproc { class process; }

class ChildProcess
{
public:
    using UCPtr = std::unique_ptr<const ChildProcess>;

    static ChildProcess::UCPtr Create(const std::vector<std::string>& args);
    ~ChildProcess();

    bool IsRunning() const noexcept;
    int GetExitStatus() const noexcept;

private:
    ChildProcess();

    template<typename T, typename SFINAE = std::enable_if_t<std::is_enum<T>::value, T>>
    static int EnumValue(const T e)
    {
        return static_cast<typename std::underlying_type<T>::type>(e);
    }

    mutable std::unique_ptr<reproc::process> m_pProcess;
};