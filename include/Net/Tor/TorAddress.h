#pragma once

#include <string>
#include <optional>

class TorAddress
{
public:
	TorAddress(const std::string& address) : m_address(address) { }

	inline const std::string& ToString() const { return m_address; }

	// 2.2.7. Client-side validation of onion addresses

	//  When a Tor client receives a prop224 onion address from the user, it
	//  MUST first validate the onion address before attempting to connect or
	//  fetch its descriptor. If the validation fails, the client MUST
	//  refuse to connect.

	//  As part of the address validation, Tor clients should check that the
	//  underlying ed25519 key does not have a torsion component. If Tor accepted
	//  ed25519 keys with torsion components, attackers could create multiple
	//  equivalent onion addresses for a single ed25519 key, which would map to the
	//  same service. We want to avoid that because it could lead to phishing
	//  attacks and surprising behaviors (e.g. imagine a browser plugin that blocks
	//  onion addresses, but could be bypassed using an equivalent onion address
	//  with a torsion component).

	//  The right way for clients to detect such fraudulent addresses (which should
	//  only occur malevolently and never natutally) is to extract the ed25519
	//  public key from the onion address and multiply it by the ed25519 group order
	//  and ensure that the result is the ed25519 identity element. For more
	//  details, please see [TORSION-REFS].
	static std::optional<TorAddress> TryParse(const std::string& address)
	{

	}

private:
	std::string m_address; // TODO: Replace with public key
};