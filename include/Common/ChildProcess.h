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

        if (ec == std::errc::no_such_file_or_directory)
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

    mutable reproc::process m_process;
};