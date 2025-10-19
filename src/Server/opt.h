#pragma once

#include <Core/Enums/Environment.h>
#include <Net/SocketAddress.h>
#include <filesystem.h>
#include <iostream>
#include <optional>

#include "ketopt.h"

struct Options
{
	bool help;
	bool headless;
	Environment environment;
	bool include_wallet;
	std::optional<std::pair<std::string, uint16_t>> shared_node;
	std::optional<fs::path> config_path;
};

static ko_longopt_t longopts[] = {
	{ "help", ko_no_argument, 301 },
	{ "floonet", ko_no_argument, 302 },
	{ "headless", ko_no_argument, 303 },
	{ "no-wallet", ko_no_argument, 304 },
	{ "shared-node", ko_required_argument, 305 },
	{ "config-path", ko_required_argument, 306 },
	{ NULL, 0, 0 }
};

static Options ParseOptions(int argc, char* argv[])
{
	Options options{
		false,
		false,
		Environment::MAINNET,
		true,
		std::nullopt,
		std::nullopt
	};

	ketopt_t opt = KETOPT_INIT;
	int c;
	while ((c = ketopt(&opt, argc, argv, 1, "xy:", longopts)) >= 0) {
		if (c == 301) options.help = true;
		else if (c == 302) options.environment = Environment::FLOONET;
		else if (c == 303) options.headless = true;
		else if (c == 304) options.include_wallet = false;
		else if (c == 305)
		{
			std::vector<std::string> parts = StringUtil::Split(opt.arg, ":");
			options.shared_node = std::make_optional<std::pair<std::string, uint16_t>>({ parts[0], (uint16_t)std::stoul(parts[1]) });
			options.headless = true;
		}
		else if (c == 306)
		{
			options.config_path = opt.arg;
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
	std::cout << "--shared-node [host:port]" << "\t" << "Uses the node at the given address and port (grinnode.live:80)" << std::endl;
	std::cout << "--config-path [path_to_config]" << "\t" << "Uses the config file at the path provided" << std::endl;
}