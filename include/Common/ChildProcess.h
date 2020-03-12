#pragma once

#include <reproc++/reproc.hpp>

#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <array>

class ChildProcess
{
public:
    using CPtr = std::shared_ptr<const ChildProcess>;

    static ChildProcess::CPtr Create(const std::vector<std::string>& args)
    {
        assert(!args.empty());

        std::shared_ptr<ChildProcess> pChildProcess = std::shared_ptr<ChildProcess>(new ChildProcess());

        reproc::stop_actions stop_actions = {
            { reproc::stop::terminate, reproc::milliseconds(5000) },
            { reproc::stop::kill, reproc::milliseconds(2000) },
            {}
        };

        reproc::options options;
        options.stop_actions = stop_actions;

        options.redirect = { reproc::redirect::inherit, reproc::redirect::inherit, reproc::redirect::inherit };

        std::error_code ec = pChildProcess->m_process.start(args, options);

        if (ec.value() == EnumValue(std::errc::no_such_file_or_directory) || ec.value() == EnumValue(std::errc::no_such_device_or_address))
        {
            return nullptr;
        }

        return pChildProcess;
    }

    ~ChildProcess()
    {
        reproc::stop_actions stop_actions = {
            { reproc::stop::terminate, reproc::milliseconds(5000) },
            { reproc::stop::kill, reproc::milliseconds(2000) },
            {}
        };
        m_process.stop(stop_actions);
    }

    bool IsRunning() const { return m_process.running(); }
    int GetExitStatus() const { return m_process.exit_status(); }

private:
    ChildProcess() = default;

    template<typename T, typename SFINAE = std::enable_if_t<std::is_enum<T>::value, T>>
    static int EnumValue(const T e)
    {
        return static_cast<typename std::underlying_type<T>::type>(e);
    }

    mutable reproc::process m_process;
};