#include "Server.h"

#include <iostream>

int main(int argc, char* argv[])
{
	std::cout << "INITIALIZING...\n";

	Server server;
	server.Run();

	return 0;
}