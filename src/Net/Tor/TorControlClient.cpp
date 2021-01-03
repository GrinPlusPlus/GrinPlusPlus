#include "TorControlClient.h"

#include <Common/Util/ThreadUtil.h>
#include <Core/Global.h>
#include <regex>

TorControlClient::UPtr TorControlClient::Connect(const uint16_t control_port, const std::string& password)
{
	auto pClient = std::make_unique<TorControlClient>();
	bool connected = false;
	auto timeout = std::chrono::system_clock::now() + TOR_STARTUP_TIMEOUT;
	while (!connected && std::chrono::system_clock::now() < timeout) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		// Open control socket
		connected = pClient->Connect(SocketAddress{ "127.0.0.1", control_port });
	}

	if (!connected) {
		LOG_WARNING_F("Failed to connect to control_port {}", control_port);
		return nullptr;
	}

	// Authenticate
	std::string auth_command = StringUtil::Format("AUTHENTICATE \"{}\"\n", password);

	try {
		pClient->Invoke(auth_command);

		for (int i = 0; i < 10; i++) {
			ThreadUtil::SleepFor(std::chrono::seconds(2));
			if (pClient->IsBootstrapped()) {
				return pClient;
			}
		}
	}
	catch (std::exception&) {
		return nullptr;
	}

	return nullptr;
}

bool TorControlClient::IsBootstrapped()
{
	const std::regex base_regex(".*PROGRESS=([0-9]+).*");
	std::smatch base_match;

	std::vector<std::string> response = Invoke("GETINFO status/bootstrap-phase\n");
	for (std::string& line : response) {
		if (std::regex_match(line, base_match, base_regex)) {
			// The first sub_match is the whole string; the next
			// sub_match is the first parenthesized expression.
			if (base_match.size() == 2) {
				int percentage = std::stoi(base_match[1].str());
				if (percentage == 100) {
					return true;
				}
			}
		}
	}

	return false;
}

std::vector<std::string> TorControlClient::Invoke(const std::string& request)
{
	assert(!request.empty());

	std::vector<std::string> response;

	try {
		Write(request.back() == '\n' ? request : request + "\n", TOR_CONTROL_TIMEOUT);

		std::string line = ReadLine(TOR_CONTROL_TIMEOUT).Trim();
		while (line != "250 OK") {
			if (!StringUtil::StartsWith(line, "250")) {
				throw TOR_EXCEPTION("Failed with error: " + line);
			}

			response.push_back(line);
			line = ReadLine(TOR_CONTROL_TIMEOUT).Trim();
		}
	}
	catch (TorException&) {
		throw;
	}
	catch (std::exception& e) {
		throw TOR_EXCEPTION(e.what());
	}

	return response;
}

bool TorControlClient::Connect(const SocketAddress& address)
{
	try {
		Client::Connect(address, TOR_CONTROL_TIMEOUT);
		return true;
	}
	catch (asio::system_error& e) {
		LOG_ERROR_F("Connection to {} failed with error {}.", address, e.what());
		return false;
	}
}