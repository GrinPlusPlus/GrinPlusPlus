#pragma once

#include <Common/Util/HexUtil.h>
#include <Crypto/BlindingFactor.h>
#include <Crypto/Commitment.h>
#include <Crypto/PublicKey.h>
#include <Crypto/Signature.h>
#include <Crypto/RangeProof.h>
#include <Core/Serialization/DeserializationException.h>
#include <json/json.h>
#include <optional>
#include <vector>

class JsonUtil
{
public:
	static Json::Value ConvertToJSON(const std::vector<unsigned char>& bytes, const bool hex)
	{
		if (hex)
		{
			return Json::Value(HexUtil::ConvertToHex(bytes));
		}

		Json::Value node;

		for (size_t i = 0; i < bytes.size(); i++)
		{
			node.append(bytes[i]);
		}

		return node;
	}

	static std::vector<unsigned char> ConvertToVector(const Json::Value& node, const size_t expectedSize, const bool hex)
	{
		if (node == Json::nullValue)
		{
			throw DeserializationException();
		}

		if (hex)
		{
			std::vector<unsigned char> bytes = HexUtil::FromHex(std::string(node.asString()));
			if (bytes.size() != expectedSize)
			{
				throw DeserializationException();
			}

			return bytes;
		}

		const size_t size = node.size();
		if (size != expectedSize)
		{
			throw DeserializationException();
		}

		std::vector<unsigned char> bytes(size);
		for (size_t i = 0; i < size; i++)
		{
			bytes[i] = (unsigned char)node.get(Json::ArrayIndex(i), Json::Value(0)).asUInt();
		}

		return bytes;
	}

	static Json::Value GetRequiredField(const Json::Value& node, const std::string& key)
	{
		if (node == Json::nullValue)
		{
			throw DeserializationException();
		}

		const Json::Value value = node.get(key, Json::nullValue);
		if (value == Json::nullValue)
		{
			throw DeserializationException();
		}

		return value;
	}

	static Json::Value GetOptionalField(const Json::Value& node, const std::string& key)
	{
		if (node == Json::nullValue)
		{
			throw DeserializationException();
		}

		return node.get(key, Json::nullValue);
	}

	//
	// BlindingFactors
	//
	static Json::Value ConvertToJSON(const BlindingFactor& blindingFactor, const bool hex)
	{
		return ConvertToJSON(blindingFactor.GetBytes().GetData(), hex);
	}

	static BlindingFactor ConvertToBlindingFactor(const Json::Value& blindingFactorJSON, const bool hex)
	{
		return BlindingFactor(CBigInteger<32>(ConvertToVector(blindingFactorJSON, 32, hex)));
	}

	static BlindingFactor GetBlindingFactor(const Json::Value& parentJSON, const std::string& key, const bool hex)
	{
		return ConvertToBlindingFactor(GetRequiredField(parentJSON, key), hex);
	}

	//
	// Commitments
	//
	static Json::Value ConvertToJSON(const Commitment& commitment, const bool hex)
	{
		return ConvertToJSON(commitment.GetCommitmentBytes().GetData(), hex);
	}

	static Commitment ConvertToCommitment(const Json::Value& commitmentJSON, const bool hex)
	{
		return Commitment(CBigInteger<33>(ConvertToVector(commitmentJSON, 33, hex)));
	}

	static Commitment GetCommitment(const Json::Value& parentJSON, const std::string& key, const bool hex)
	{
		return ConvertToCommitment(GetRequiredField(parentJSON, key), hex);
	}

	//
	// PublicKeys
	//
	static Json::Value ConvertToJSON(const PublicKey& publicKey, const bool hex)
	{
		return ConvertToJSON(publicKey.GetCompressedBytes().GetData(), hex);
	}

	static PublicKey ConvertToPublicKey(const Json::Value& publicKeyJSON, const bool hex)
	{
		return PublicKey(CBigInteger<33>(ConvertToVector(publicKeyJSON, 33, hex)));
	}

	static PublicKey GetPublicKey(const Json::Value& parentJSON, const std::string& key, const bool hex)
	{
		return ConvertToPublicKey(GetRequiredField(parentJSON, key), hex);
	}

	//
	// Signatures
	//
	static Json::Value ConvertToJSON(const Signature& signature, const bool hex)
	{
		return ConvertToJSON(signature.GetSignatureBytes().GetData(), hex);
	}

	static Signature ConvertToSignature(const Json::Value& signatureJSON, const bool hex)
	{
		return Signature(CBigInteger<64>(ConvertToVector(signatureJSON, 64, hex)));
	}

	static Signature GetSignature(const Json::Value& parentJSON, const std::string& key, const bool hex)
	{
		return ConvertToSignature(GetRequiredField(parentJSON, key), hex);
	}

	static Json::Value ConvertToJSON(const std::optional<Signature>& signatureOpt, const bool hex)
	{
		return signatureOpt.has_value() ? ConvertToJSON(signatureOpt.value(), hex) : Json::nullValue;
	}

	static std::optional<Signature> ConvertToSignatureOpt(const Json::Value& signatureJSON, const bool hex)
	{
		if (signatureJSON == Json::nullValue)
		{
			return std::nullopt;
		}

		return std::make_optional<Signature>(Signature(CBigInteger<64>(ConvertToVector(signatureJSON, 64, hex))));
	}

	static std::optional<Signature> GetSignatureOpt(const Json::Value& parentJSON, const std::string& key, const bool hex)
	{
		return ConvertToSignatureOpt(GetOptionalField(parentJSON, key), hex);
	}

	//
	// RangeProofs
	//
	static Json::Value ConvertToJSON(const RangeProof& rangeProof, const bool hex)
	{
		return ConvertToJSON(rangeProof.GetProofBytes(), hex);
	}

	static RangeProof ConvertToRangeProof(const Json::Value& rangeProofJSON, const bool hex)
	{
		return RangeProof(ConvertToVector(rangeProofJSON, MAX_PROOF_SIZE, hex));
	}

	static RangeProof GetRangeProof(const Json::Value& parentJSON, const std::string& key, const bool hex)
	{
		return ConvertToRangeProof(GetRequiredField(parentJSON, key), hex);
	}

	//
	// Strings
	//
	static Json::Value ConvertToJSON(const std::optional<std::string>& stringOpt)
	{
		return stringOpt.has_value() ? Json::Value(stringOpt.value()) : Json::nullValue;
	}

	static std::optional<std::string> ConvertToStringOpt(const Json::Value& stringJSON)
	{
		return stringJSON == Json::nullValue ? std::nullopt : std::make_optional<std::string>(stringJSON.asString());
	}

	static std::optional<std::string> GetStringOpt(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToStringOpt(GetOptionalField(parentJSON, key));
	}

	//
	// UInt64
	//
	static uint64_t ConvertToUInt64(const Json::Value& uint64JSON)
	{
		if (uint64JSON.isUInt64())
		{
			return uint64JSON.asUInt64();
		}
		else if (uint64JSON.isString())
		{
			return std::stoull(uint64JSON.asString());
		}
		else
		{
			throw DeserializationException();
		}
	}

	static uint64_t GetRequiredUInt64(const Json::Value& parentJSON, const std::string& key)
	{
		const Json::Value value = JsonUtil::GetRequiredField(parentJSON, key);
		return ConvertToUInt64(value);
	}

	static std::optional<uint64_t> GetUInt64Opt(const Json::Value& parentJSON, const std::string& key)
	{
		const Json::Value value = JsonUtil::GetOptionalField(parentJSON, key);
		if (value.isNull())
		{
			return std::nullopt;
		}
		else
		{
			return ConvertToUInt64(value);
		}
	}

	//
	// Optional
	//
	template<class T>
	static void AddOptionalField(Json::Value& json, const std::string& fieldName, const std::optional<T>& opt)
	{
		if (opt.has_value())
		{
			json[fieldName] = opt.value();
		}
	}

	// Parse
	static bool Parse(const std::string& input, Json::Value& outputJSON)
	{
		std::string errors;
		std::istringstream inputStream(input);
		return Json::parseFromStream(Json::CharReaderBuilder(), inputStream, &outputJSON, &errors);
	}
};