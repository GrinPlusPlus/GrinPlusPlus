#pragma once

#include <Config/EnvironmentType.h>
#include <Net/SocketAddress.h>
#include <klib/ketopt.h>
#include <iostream>
#include <optional>

struct Options
{
	bool help;
	bool headless;
	EEnvironmentType environment;
	bool include_wallet;
	std::optional<SocketAddress> shared_node;
};

static ko_longopt_t longopts[] = {
	{ "help", ko_no_argument, 301 },
	{ "floonet", ko_no_argument, 302 },
	{ "headless", ko_no_argument, 303 },
	{ "no-wallet", ko_no_argument, 304 },
	{ "shared-node", ko_required_argument, 305 },
	{ NULL, 0, 0 }
};

static Options ParseOptions(int argc, char* argv[])
{
	Options options{
		false,
		false,
		EEnvironmentType::MAINNET,
		true,
		std::nullopt
	};

	ketopt_t opt = KETOPT_INIT;
	int c;
	while ((c = ketopt(&opt, argc, argv, 1, "xy:", longopts)) >= 0) {
		if (c == 301) options.help = true;
		else if (c == 302) options.environment = EEnvironmentType::FLOONET;
		else if (c == 303) options.headless = true;
		else if (c == 304) options.include_wallet = false;
		else if (c == 305)
		{
			options.shared_node = std::make_optional<SocketAddress>(SocketAddress::Parse(opt.arg));
			options.headless = true;
		}
	}

	return options;
}

static void PrintHelp()
{
	std::cout << "Usage: GrinPP [options]" << std::endl;
	std::cout << "--help" << "\t" << "Display this help message" << std::endl;
	std::cout << "--floonet" << "\t" << "Use floonet chain" << std::endl;
	std::cout << "--headless" << "\t" << "Run headless app with no console output" << std::endl;
	std::cout << "--no-wallet" << "\t" << "Run only a node, not a wallet" << std::endl;
	std::cout << "--shared-node [IP:port]" << "\t" << "Uses the node at a given IP address and port (###.###.###.###:####)" << std::endl;
}