#pragma once

#include <inttypes.h>
#include <sodium.h>
#include <Common/Util/HexUtil.h>
#include <Crypto/Crypto.h>
#include <Crypto/CryptoException.h>
#include <Crypto/SecretKey.h>
#include <Crypto/Signature.h>
#include <vector>

#define ED25519_PUBKEY_LEN 32

/** An Ed25519 public key */
struct ed25519_public_key_t
{
	ed25519_public_key_t() = default;
	ed25519_public_key_t(CBigInteger<32>&& bytes_) : bytes(std::move(bytes_)) { }
	ed25519_public_key_t(const CBigInteger<32>& bytes_) : bytes(bytes_) { }

	bool operator==(const ed25519_public_key_t& rhs) const { return bytes == rhs.bytes; }
	bool operator!=(const ed25519_public_key_t& rhs) const { return bytes != rhs.bytes; }

	unsigned char* data() noexcept { return bytes.data(); }
	const unsigned char* data() const noexcept { return bytes.data(); }
	const std::vector<uint8_t>& vec() const noexcept { return bytes.GetData(); }

	std::vector<uint8_t>::const_iterator cbegin() const noexcept { return vec().cbegin(); }
	std::vector<uint8_t>::const_iterator cend() const noexcept { return vec().cend(); }

	std::string Format() const { return bytes.ToHex(); }

	CBigInteger<32> bytes;
};

/*
 * An Ed25519 secret key
 * Encoding format: [32 bytes seed | 32 bytes public key]
 */
struct ed25519_secret_key_t
{
	ed25519_secret_key_t() = default;
	ed25519_secret_key_t(SecretKey64&& bytes_) : bytes(std::move(bytes_)) { }
	ed25519_secret_key_t(const SecretKey64& bytes_) : bytes(bytes_) { }

	bool operator==(const ed25519_secret_key_t& rhs) const { return bytes == rhs.bytes; }
	bool operator!=(const ed25519_secret_key_t& rhs) const { return bytes != rhs.bytes; }

	unsigned char* data() noexcept { return bytes.data(); }
	const unsigned char* data() const noexcept { return bytes.data(); }
	const std::vector<uint8_t>& vec() const noexcept { return bytes.GetVec(); }

	std::vector<uint8_t>::const_iterator cbegin() const noexcept { return vec().cbegin(); }
	std::vector<uint8_t>::const_iterator cend() const noexcept { return vec().cend(); }

	SecretKey64 bytes;
};

struct ed25519_keypair_t
{
	ed25519_keypair_t() = default;
	ed25519_keypair_t(ed25519_public_key_t&& public_key_, ed25519_secret_key_t&& secret_key_)
		: public_key(std::move(public_key_)), secret_key(std::move(secret_key_)) { }

	ed25519_public_key_t public_key;
	ed25519_secret_key_t secret_key;
};

class ED25519
{
public:
	static ed25519_public_key_t ToPubKey(const std::vector<uint8_t>& bytes)
	{
		if (bytes.size() != ED25519_PUBKEY_LEN) {
			throw std::exception();
		}

		ed25519_public_key_t pubkey;
		pubkey.bytes = bytes;
		if (!IsValid(pubkey)) {
			throw std::exception();
		}

		return pubkey;
	}

	static bool IsValid(const ed25519_public_key_t& pubkey)
	{
		return crypto_core_ed25519_is_valid_point(pubkey.data()) != 0;
	}

	/* Return true if <b>point</b> is the identity element of the ed25519 group. */
	static bool IsIdentityElement(const ed25519_public_key_t& point)
	{
		/* The identity element in ed25159 is the point with coordinates (0,1). */
		static const uint8_t ed25519_identity[32] = {
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
		};

		return tor_memeq(point.data(), ed25519_identity, sizeof(ed25519_identity)) != 0;
	}

	//static ed25519_public_key_t MultiplyWithGroupOrder(const ed25519_public_key_t& pubKey)
	//{
	//	ed25519_public_key_t result;
	//	const int status = crypto_scalarmult_ed25519(result.data(), pubKey.data());
	//	if (status != 0)
	//	{
	//		throw CryptoException("ED25519::MultiplyWithGroupOrder");
	//	}

	//	return result;
	//}

	static ed25519_keypair_t CalculateKeypair(const SecretKey& seed)
	{
		ed25519_public_key_t public_key;
		ed25519_secret_key_t secret_key;
		const int status = crypto_sign_ed25519_seed_keypair(public_key.data(), secret_key.data(), seed.data());
		if (status != 0) {
			throw CryptoException("ED25519::CalculateKeypair");
		}

		return ed25519_keypair_t(std::move(public_key), std::move(secret_key));
	}

	static ed25519_public_key_t CalculatePubKey(const ed25519_secret_key_t& secretKey)
	{
		ed25519_public_key_t result;
		const int status = crypto_sign_ed25519_sk_to_pk(result.data(), secretKey.data());
		if (status != 0)
		{
			throw CryptoException("ED25519::CalculatePubKey");
		}

		return result;
	}

	static bool VerifySignature(const ed25519_public_key_t& publicKey, const Signature& signature, const std::vector<unsigned char>& message)
	{
		std::vector<uint8_t> signature_and_message;
		signature_and_message.insert(signature_and_message.end(), signature.cbegin(), signature.cend());
		signature_and_message.insert(signature_and_message.end(), message.cbegin(), message.cend());
		std::vector<uint8_t> message_out(message.size());
		unsigned long long message_len = message_out.size();

		const int status = crypto_sign_open(message_out.data(), &message_len, signature_and_message.data(), signature_and_message.size(), publicKey.data());

		return status == 0;
	}

	static Signature Sign(const ed25519_secret_key_t& secretKey, const std::vector<unsigned char>& message)
	{
		std::vector<uint8_t> signature_bytes(64 + message.size());
		unsigned long long signature_size = signature_bytes.size();
		const int status = crypto_sign_ed25519(signature_bytes.data(), &signature_size, message.data(), message.size(), secretKey.data());
		if (status != 0 || signature_size < 64)
		{
			throw CryptoException("ED25519::Sign");
		}

		return Signature(CBigInteger<64>(signature_bytes.data()));
	}

	// Given an ed25519_secret_key_t, calculates the 32-byte secret scalar (a) and the PRF secret (RH).
	// See: http://ffp4g1ylyit3jdyti1hqcvtb-wpengine.netdna-ssl.com/warner/files/2011/11/key-formats.png
	static SecretKey64 CalculateTorKey(const ed25519_secret_key_t& secretKey)
	{
		SecretKey64 torKey;
		crypto_hash_sha512(torKey.data(), secretKey.data(), 32);
		torKey[0] &= 248;
		torKey[31] &= 127;
		torKey[31] |= 64;
		return torKey;
	}

private:
	/**
	 * Timing-safe memory comparison.  Return true if the <b>sz</b> bytes at
	 * <b>a</b> are the same as the <b>sz</b> bytes at <b>b</b>, and 0 otherwise.
	 *
	 * This implementation differs from !memcmp(a,b,sz) in that its timing
	 * behavior is not data-dependent: it should return in the same amount of time
	 * regardless of the contents of <b>a</b> and <b>b</b>.  It differs from
	 * !tor_memcmp(a,b,sz) by being faster.
	 */
	static int tor_memeq(const void* a, const void* b, size_t sz)
	{
		/* Treat a and b as byte ranges. */
		const uint8_t* ba = (const uint8_t*)a, * bb = (const uint8_t*)b;
		uint32_t any_difference = 0;
		while (sz--) {
			/* Set byte_diff to all of those bits that are different in *ba and *bb,
			 * and advance both ba and bb. */
			const uint8_t byte_diff = *ba++ ^ *bb++;

			/* Set bits in any_difference if they are set in byte_diff. */
			any_difference |= byte_diff;
		}

		/* Now any_difference is 0 if there are no bits different between
		 * a and b, and is nonzero if there are bits different between a
		 * and b.  Now for paranoia's sake, let's convert it to 0 or 1.
		 *
		 * (If we say "!any_difference", the compiler might get smart enough
		 * to optimize-out our data-independence stuff above.)
		 *
		 * To unpack:
		 *
		 * If any_difference == 0:
		 *            any_difference - 1 == ~0
		 *     (any_difference - 1) >> 8 == 0x00ffffff
		 *     1 & ((any_difference - 1) >> 8) == 1
		 *
		 * If any_difference != 0:
		 *            0 < any_difference < 256, so
		 *            0 <= any_difference - 1 < 255
		 *            (any_difference - 1) >> 8 == 0
		 *            1 & ((any_difference - 1) >> 8) == 0
		 */

		 /*coverity[overflow]*/
		return 1 & ((any_difference - 1) >> 8);
	}
};