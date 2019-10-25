#pragma once

#include <Config/Config.h>
#include <Net/SocketAddress.h>

#include <string>
#include <vector>

// Forward Declarations
struct in_addr;

//
// This is in charge of communication with MimbleWimble dns seeds.
//
class DNSSeeder
{
  public:
    DNSSeeder(const Config &config);

    //
    // Connects to one of the "trusted" DNS seeds, and retrieves a collection of IP addresses to MimbleWimble nodes.
    //
    std::vector<SocketAddress> GetPeersFromDNS() const;

  private:
    const Config &m_config;

    std::vector<IPAddress> Resolve(const std::string &domainName) const;
};