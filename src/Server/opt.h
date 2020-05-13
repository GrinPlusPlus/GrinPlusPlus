#pragma once

#include <Config/EnvironmentType.h>
#include <Net/SocketAddress.h>
#include <boost/program_options.hpp>
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

static boost::program_options::options_description GetDescription()
{
	namespace po = boost::program_options;

	po::options_description description("Usage: GrinPP [options]");

	// TODO: override config file path
	description.add_options()
		("help,h", "Display this help message")
		("floonet", "Use floonet chain")
		("headless", po::value<std::string>(), "Run headless app with no console output")
		("node-only", "Run only a node, not a wallet")
		("shared-node", po::value<std::string>(), "Runs only a wallet, and uses the node at a given IP address and port (###.###.###.###:####)");

	return description;
}

static Options ParseOptions(int argc, char* argv[])
{
	namespace po = boost::program_options;

	auto description = GetDescription();

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(description).run(), vm);
	po::notify(vm);

	return Options{
		vm.count("help") > 0,
		vm.count("headless") > 0 || vm.count("shared-node") > 0,
		vm.count("floonet") > 0 ? EEnvironmentType::FLOONET : EEnvironmentType::MAINNET,
		vm.count("node-only") == 0,
		vm.count("shared-node") > 0 ? std::make_optional<SocketAddress>(SocketAddress::Parse(vm.at("shared-node").as<std::string>())) : std::nullopt
	};
}

static void PrintHelp()
{
	std::cout << GetDescription() << std::endl;
}