#pragma once

#include <Net/SocketAddress.h>
#include <Net/Tor/TorAddress.h>
#include <vector>

struct TorStatus
{
	bool liveness;
	bool circuit_established;
	bool enough_dir_info;

	std::string config_file;
	SocketAddress socks_listener;
	SocketAddress control_listener;

	std::string bootstrap_phase;
	size_t bytes_read;
	size_t bytes_written;
	size_t uptime;

	int process_pid;
	IPAddress external_ip;

	std::string ToString() const
	{
		std::vector<std::string> status_lines;

		status_lines.push_back(std::string{ "Network-Liveness: " } + (liveness ? "UP" : "DOWN"));

		// TODO: Finish this
	}
};