#pragma once

#include "spdlog/spdlog.h"

#include <memory>
#include <string>

class Logger
{
  public:
    static Logger &GetInstance();

    void StartLogger(const std::string &directory, const spdlog::level::level_enum &logLevel);
    void Log(const spdlog::level::level_enum logLevel, const std::string &eventText);
    void Flush();

  private:
    Logger();

    std::shared_ptr<spdlog::logger> m_pLogger;
};