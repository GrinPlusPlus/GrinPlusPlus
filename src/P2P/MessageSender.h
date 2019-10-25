#pragma once

#include "Messages/Message.h"

#include <Config/Config.h>
#include <Net/Socket.h>

class MessageSender
{
  public:
    MessageSender(const Config &config);

    bool Send(Socket &socket, const IMessage &message) const;

  private:
    const Config &m_config;
};