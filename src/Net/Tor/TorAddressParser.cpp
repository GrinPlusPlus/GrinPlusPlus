#include <Net/Tor/TorAddressParser.h>

#include <Common/Logger.h>
#include <Crypto/ED25519.h>
#include <Crypto/Models/BigInteger.h>
#include <sha3/sha3.h>
#include <cppcodec/base32_rfc4648.hpp>

/* Compute the CEIL of <b>a</b> divided by <b>b</b>, for nonnegative <b>a</b>
 * and positive <b>b</b>.  Works on integer types only. Not defined if a+(b-1)
 * can overflow. */
#define CEIL_DIV(a,b) (((a)+((b)-1))/(b))
 /** Length of the output of our second (improved) message digests.  (For now
  * this is just sha256, but it could be any other 256-bit digest.) */
#define DIGEST256_LEN 32
  /* Prefix of the onion address checksum. */
#define HS_SERVICE_ADDR_CHECKSUM_PREFIX ".onion checksum"
/* Length of the checksum prefix minus the NUL terminated byte. */
#define HS_SERVICE_ADDR_CHECKSUM_PREFIX_LEN \
  (sizeof(HS_SERVICE_ADDR_CHECKSUM_PREFIX) - 1)
/* Length of the resulting checksum of the address. The construction of this
 * checksum looks like:
 *   CHECKSUM = ".onion checksum" || PUBKEY || VERSION
 * where VERSION is 1 byte. This is pre-hashing. */
#define HS_SERVICE_ADDR_CHECKSUM_INPUT_LEN \
  (HS_SERVICE_ADDR_CHECKSUM_PREFIX_LEN + ED25519_PUBKEY_LEN + sizeof(uint8_t))
 /* The amount of bytes we use from the address checksum. */
#define HS_SERVICE_ADDR_CHECKSUM_LEN_USED 2
/* Length of the binary encoded service address which is of course before the
 * base32 encoding. Construction is:
 *    PUBKEY || CHECKSUM || VERSION
 * with 1 byte VERSION and 2 bytes CHECKSUM. The following is 35 bytes. */
#define HS_SERVICE_ADDR_LEN \
  (ED25519_PUBKEY_LEN + HS_SERVICE_ADDR_CHECKSUM_LEN_USED + sizeof(uint8_t))
 /* Length of 'y' portion of 'y.onion' URL. This is base32 encoded and the
  * length ends up to 56 bytes (not counting the terminated NUL byte.) */
#define HS_SERVICE_ADDR_LEN_BASE32 \
  (CEIL_DIV(HS_SERVICE_ADDR_LEN * 8, 5))

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
std::optional<TorAddress> TorAddressParser::Parse(const std::string& address)
{
	try
	{
		/* Obvious length check. */
		if (address.size() != HS_SERVICE_ADDR_LEN_BASE32) {
			return std::nullopt;
		}

		/* Decode address so we can extract needed fields. */
		std::vector<uint8_t> b32Decoded = cppcodec::base32_rfc4648::decode(address);
		if (b32Decoded.size() != HS_SERVICE_ADDR_LEN) {
			return std::nullopt;
		}

		/* Parse the decoded address into the fields we need. */
		size_t offset = 0;

		/* First is the key. */
		std::vector<uint8_t> pubkey;
		pubkey.insert(pubkey.begin(), b32Decoded.cbegin(), b32Decoded.cbegin() + ED25519_PUBKEY_LEN);
		offset += ED25519_PUBKEY_LEN;

		/* Followed by a 2 bytes checksum. */
		std::vector<uint8_t> checksum(
			b32Decoded.begin() + offset,
			b32Decoded.begin() + offset + HS_SERVICE_ADDR_CHECKSUM_LEN_USED
		);
		offset += HS_SERVICE_ADDR_CHECKSUM_LEN_USED;

		/* Finally, version value is 1 byte. */
		uint8_t version = b32Decoded[offset];
		offset += sizeof(uint8_t);

		/* Extra safety. */
		if (offset != HS_SERVICE_ADDR_LEN) {
			return std::nullopt;
		}

		ed25519_public_key_t publicKey = CBigInteger<32>(std::move(pubkey));
		if (IsValid(version, publicKey, checksum)) {
			return std::make_optional<TorAddress>(address, publicKey, version);
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
		return std::nullopt;
	}

	return std::nullopt;
}

TorAddress TorAddressParser::FromPubKey(const ed25519_public_key_t& pubkey)
{
	uint8_t version = 3;

	std::vector<uint8_t> checksum;
	BuildChecksum(pubkey, version, checksum);
	LOG_INFO_F("Checksum: {} - {}", HexUtil::ConvertToHex(checksum), checksum.size());

	std::vector<uint8_t> bytes = pubkey.vec();
	bytes.insert(bytes.end(), checksum.cbegin(), checksum.cend());
	bytes.push_back(version);

	return TorAddress(StringUtil::ToLower(cppcodec::base32_rfc4648::encode(bytes)), pubkey, version);
}

bool TorAddressParser::IsValid(const uint8_t version, const ed25519_public_key_t& pubkey, const std::vector<uint8_t>& checksum)
{
	/* Get the checksum it's supposed to be and compare it with what we have encoded in the address. */
	std::vector<uint8_t> expectedChecksum;

	const bool checksumBuilt = BuildChecksum(pubkey, version, expectedChecksum);
	if (!checksumBuilt || CBigInteger<2>(checksum) != CBigInteger<2>(expectedChecksum)) {
		LOG_WARNING_F(
			"Checksum invalid for TOR address. Expected {} but got {}",
			CBigInteger<2>(checksum).ToHex(),
			CBigInteger<2>(expectedChecksum).ToHex()
		);
		return false;
	}

	/* Validate that this pubkey does not have a torsion component. We need to do
		* this on the prop224 client-side so that attackers can't give equivalent
		* forms of an onion address to users. */

	return ED25519::IsValid(pubkey);
}

/* Using an ed25519 public key and version to build the checksum of an
	* address. Put in checksum_out. Format is:
	*    SHA3-256(".onion checksum" || PUBKEY || VERSION)
	*
	* checksum_out must be large enough to receive 32 bytes (DIGEST256_LEN). */
bool TorAddressParser::BuildChecksum(const ed25519_public_key_t& key, uint8_t version, std::vector<uint8_t>& checksum_out)
{
	std::vector<uint8_t> vec;

	std::string prefix = ".onion checksum";
	vec.insert(vec.end(), prefix.cbegin(), prefix.cend());
	vec.insert(vec.end(), key.cbegin(), key.cend());
	vec.push_back(version);

	/* Hash the data payload to create the checksum. */
	std::string hashHex = SHA3(SHA3::Bits256)(vec.data(), HS_SERVICE_ADDR_CHECKSUM_INPUT_LEN);
	if (hashHex.size() < (HS_SERVICE_ADDR_CHECKSUM_LEN_USED * 2)) {
		return false;
	}

	checksum_out = CBigInteger<HS_SERVICE_ADDR_CHECKSUM_LEN_USED>::FromHex(hashHex.substr(0, HS_SERVICE_ADDR_CHECKSUM_LEN_USED * 2)).GetData();

	return true;
}