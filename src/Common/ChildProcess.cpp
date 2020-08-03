#include <Common/ChildProcess.h>

#include <reproc++/reproc.hpp>
#include <Common/Logger.h>
#include <cassert>

ChildProcess::UCPtr ChildProcess::Create(const std::vector<std::string>& args)
{
    assert(!args.empty());

    std::unique_ptr<ChildProcess> pChildProcess = std::unique_ptr<ChildProcess>(new ChildProcess());

    reproc::stop_actions stop_actions = {
        { reproc::stop::terminate, reproc::milliseconds(5000) },
        { reproc::stop::kill, reproc::milliseconds(2000) },
        {}
    };

    reproc::options options;
    options.stop_actions = stop_actions;

    options.redirect = { reproc::redirect::inherit, reproc::redirect::inherit, reproc::redirect::inherit };

    //#ifdef _WIN32
    //        std::vector<std::wstring> arguments;
    //        for (const auto& arg : args)
    //        {
    //            arguments.push_back(StringUtil::ToWide(arg));
    //        }
    //#else
    //        std::vector<std::string> arguments = args;
    //#endif

    std::error_code ec = pChildProcess->m_pProcess->start(args, options);

    if (ec.value() == EnumValue(std::errc::no_such_file_or_directory) || ec.value() == EnumValue(std::errc::no_such_device_or_address))
    {
        LOG_ERROR_F("Failed to open process: {} - Error: {}:{}", args[0], ec.value(), ec.message());
        return nullptr;
    }
    else if (ec)
    {
        LOG_ERROR_F("Error while opening process: {}, Error: {}:{}", args[0], ec.value(), ec.message());

        uint8_t buffer[4096];
        unsigned int bytesRead = 0;
        reproc::reproc_stream stream = reproc::reproc_stream::out;
        pChildProcess->m_pProcess->read(&stream, buffer, 4096, &bytesRead);
        if (bytesRead > 0)
        {
            std::string out((const char*)buffer, (const char*)buffer + bytesRead);
            LOG_ERROR_F("STDOUT: {}", out);
        }

        return nullptr;
    }

    return pChildProcess;
}

ChildProcess::ChildProcess()
    : m_pProcess(std::make_unique<reproc::process>(reproc::process()))
{

}

ChildProcess::~ChildProcess()
{
    reproc::stop_actions stop_actions = {
        { reproc::stop::terminate, reproc::milliseconds(5000) },
        { reproc::stop::kill, reproc::milliseconds(2000) },
        {}
    };
    std::error_code ec = m_pProcess->stop(stop_actions);
    LOG_INFO_F("reproc.stop returned: {}", ec.message());
}

bool ChildProcess::IsRunning() const noexcept
{
    return m_pProcess->running();
}

int ChildProcess::GetExitStatus() const noexcept
{
    return  m_pProcess->exit_status();
}