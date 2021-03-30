#include <Crypto/KDF.h>
#include <Core/Exceptions/CryptoException.h>
#include <Crypto/Hasher.h>
#include <Crypto/Models/ScryptParameters.h>

#include <scrypt/crypto_scrypt.h>
#include <sodium/crypto_generichash_blake2b.h>
#include <sha.h>

SecretKey KDF::PBKDF(const SecureString& password, const std::vector<uint8_t>& salt, const ScryptParameters& parameters)
{
	SecureVector buffer(64);
	if (crypto_scrypt((const uint8_t*)password.data(), password.size(), salt.data(), salt.size(), parameters.N, parameters.r, parameters.p, buffer.data(), buffer.size()) == 0)
	{
		return SecretKey(Hasher::Blake2b(buffer.data(), buffer.size()));
	}

	throw CryptoException();
}

SecretKey KDF::HKDF(const std::optional<std::vector<uint8_t>>& saltOpt,
	const std::string& label,
	const std::vector<uint8_t>& inputKeyingMaterial)
{
	SecretKey output;

	//std::vector<uint8_t> info(label.cbegin(), label.cend());
	const int result = hkdf(
		SHAversion::SHA256,
		saltOpt.has_value() ? saltOpt.value().data() : nullptr,
		saltOpt.has_value() ? (int)saltOpt.value().size() : 0,
		inputKeyingMaterial.data(),
		(int)inputKeyingMaterial.size(),
		(const uint8_t*)label.data(),
		(int)label.size(),
		output.data(),
		(int)output.size()
	);
	if (result != shaSuccess) {
		throw CryptoException();
	}

	return output;
}