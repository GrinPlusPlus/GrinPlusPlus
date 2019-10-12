#pragma once

#include <Wallet/Tor/TorAddress.h>
#include <Crypto/ED25519.h>
#include <keccak-tiny/keccak-tiny.h>

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

class TorAddressParser
{
public:
	static std::optional<TorAddress> Parse(const std::string& address)
	{
		uint8_t version;
		uint8_t checksum[HS_SERVICE_ADDR_CHECKSUM_LEN_USED];
		ed25519_public_key_t service_pubkey;

		char decoded[HS_SERVICE_ADDR_LEN];

		/* Obvious length check. */
		if (address.size() != HS_SERVICE_ADDR_LEN_BASE32) {
			//log_warn(LD_REND, "Service address %s has an invalid length. "
			//	"Expected %lu but got %lu.",
			//	escaped_safe_str(address),
			//	(unsigned long)HS_SERVICE_ADDR_LEN_BASE32,
			//	(unsigned long)strlen(address));
			return std::nullopt;
		}

		/* Decode address so we can extract needed fields. */
		if (base32_decode(decoded, sizeof(decoded), address, strlen(address)) != sizeof(decoded)) {
			//log_warn(LD_REND, "Service address %s can't be decoded.",
			//	escaped_safe_str(address));
			return std::nullopt;
		}

		/* Parse the decoded address into the fields we need. */
		size_t offset = 0;

		/* First is the key. */
		memcpy(service_pubkey.pubkey, decoded, ED25519_PUBKEY_LEN);
		offset += ED25519_PUBKEY_LEN;

		/* Followed by a 2 bytes checksum. */
		memcpy(&checksum, decoded + offset, HS_SERVICE_ADDR_CHECKSUM_LEN_USED);
		offset += HS_SERVICE_ADDR_CHECKSUM_LEN_USED;

		/* Finally, version value is 1 byte. */
		version = *(const uint8_t*)(decoded + offset);
		offset += sizeof(uint8_t);

		/* Extra safety. */
		if (offset != HS_SERVICE_ADDR_LEN)
		{
			return std::nullopt;
		}

		if (IsValid(version, pubkey, checksum))
		{
			return std::make_optional<TorAddress>(TorAddress(address)); // TODO: Pass version & pubkey
		}

		return std::nullopt;
	}

private:
	static bool IsValid(const uint8_t version, const ed25519_public_key_t& pubkey, const uint8_t* checksum)
	{
		/* Get the checksum it's supposed to be and compare it with what we have encoded in the address. */
		uint8_t target_checksum[DIGEST256_LEN];

		const bool checksum = BuildChecksum(pubkey, version, target_checksum);
		if (!checksum || tor_memcmp(checksum, target_checksum, HS_SERVICE_ADDR_CHECKSUM_LEN_USED) != 0)
		{
			//log_warn(LD_REND, "Service address %s invalid checksum.",
			//	escaped_safe_str(address));
			return false;
		}

		/* Validate that this pubkey does not have a torsion component. We need to do
		 * this on the prop224 client-side so that attackers can't give equivalent
		 * forms of an onion address to users. */

		/* First check that we were not given the identity element */
		if (ED25519::IsIdentityElement(pubkey))
		{
			//log_warn(LD_CRYPTO, "ed25519 pubkey is the identity");
			return false;
		}

		/* For any point on the curve, doing l*point should give the identity element
		 * (where l is the group order). Do the computation and check that the
		 * identity element is returned. */
		if (!ED25519::MultiplyWithGroupOrder(pubkey))
		{
			//log_warn(LD_CRYPTO, "ed25519 group order scalarmult failed");
			return false;
		}

		if (!ED25519::IsIdentityElement(result))
		{
			//log_warn(LD_CRYPTO, "ed25519 validation failed");
			return false;
		}

		return true;
	}

	/* Using an ed25519 public key and version to build the checksum of an
	 * address. Put in checksum_out. Format is:
	 *    SHA3-256(".onion checksum" || PUBKEY || VERSION)
	 *
	 * checksum_out must be large enough to receive 32 bytes (DIGEST256_LEN). */
	static bool BuildChecksum(const ed25519_public_key_t& key, uint8_t version, uint8_t* checksum_out)
	{
		size_t offset = 0;
		char data[HS_SERVICE_ADDR_CHECKSUM_INPUT_LEN];

		/* Build checksum data. */
		memcpy(data, HS_SERVICE_ADDR_CHECKSUM_PREFIX, HS_SERVICE_ADDR_CHECKSUM_PREFIX_LEN);
		offset += HS_SERVICE_ADDR_CHECKSUM_PREFIX_LEN;

		memcpy(data + offset, key.pubkey, ED25519_PUBKEY_LEN);
		offset += ED25519_PUBKEY_LEN;

		*(uint8_t*)(data + offset) = version;
		offset += sizeof(version);

		if (offset != HS_SERVICE_ADDR_CHECKSUM_INPUT_LEN)
		{
			return false;
		}

		/* Hash the data payload to create the checksum. */
		return sha3_256((uint8_t*)checksum_out, DIGEST256_LEN, (const uint8_t*)data, sizeof(data)) == 0;
	}
};