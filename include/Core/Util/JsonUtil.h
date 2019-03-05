#pragma once

#include <Crypto/BlindingFactor.h>
#include <Crypto/Commitment.h>
#include <Crypto/Signature.h>
#include <Crypto/RangeProof.h>
#include <Core/Serialization/DeserializationException.h>
#include <json/json.h>
#include <optional>
#include <vector>

class JsonUtil
{
public:
	static Json::Value ConvertToJSON(const std::vector<unsigned char>& bytes)
	{
		Json::Value node;

		for (size_t i = 0; i < bytes.size(); i++)
		{
			node.append(bytes[i]);
		}

		return node;
	}

	static std::vector<unsigned char> ConvertToVector(const Json::Value& node, const size_t expectedSize)
	{
		if (node == Json::nullValue)
		{
			throw DeserializationException();
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
	static Json::Value ConvertToJSON(const BlindingFactor& blindingFactor)
	{
		return ConvertToJSON(blindingFactor.GetBytes().GetData());
	}

	static BlindingFactor ConvertToBlindingFactor(const Json::Value& blindingFactorJSON)
	{
		return BlindingFactor(CBigInteger<32>(ConvertToVector(blindingFactorJSON, 32)));
	}

	static BlindingFactor GetBlindingFactor(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToBlindingFactor(GetRequiredField(parentJSON, key));
	}

	//
	// Commitments
	//
	static Json::Value ConvertToJSON(const Commitment& commitment)
	{
		return ConvertToJSON(commitment.GetCommitmentBytes().GetData());
	}

	static Commitment ConvertToCommitment(const Json::Value& commitmentJSON)
	{
		return Commitment(CBigInteger<33>(ConvertToVector(commitmentJSON, 33)));
	}

	static Commitment GetCommitment(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToCommitment(GetRequiredField(parentJSON, key));
	}

	//
	// Signatures
	//
	static Json::Value ConvertToJSON(const Signature& signature)
	{
		return ConvertToJSON(signature.GetSignatureBytes().GetData());
	}

	static Signature ConvertToSignature(const Json::Value& signatureJSON)
	{
		return Signature(CBigInteger<64>(ConvertToVector(signatureJSON, 64)));
	}

	static Signature GetSignature(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToSignature(GetRequiredField(parentJSON, key));
	}

	static Json::Value ConvertToJSON(const std::optional<Signature>& signatureOpt)
	{
		return signatureOpt.has_value() ? ConvertToJSON(signatureOpt.value().GetSignatureBytes().GetData()) : Json::nullValue;
	}

	static std::optional<Signature> ConvertToSignatureOpt(const Json::Value& signatureJSON)
	{
		return signatureJSON == Json::nullValue ? std::nullopt : std::make_optional<Signature>(Signature(CBigInteger<64>(ConvertToVector(signatureJSON, 64))));
	}

	static std::optional<Signature> GetSignatureOpt(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToSignatureOpt(GetOptionalField(parentJSON, key));
	}

	//
	// RangeProofs
	//
	static Json::Value ConvertToJSON(const RangeProof& rangeProof)
	{
		return ConvertToJSON(rangeProof.GetProofBytes());
	}

	static RangeProof ConvertToRangeProof(const Json::Value& rangeProofJSON)
	{
		return RangeProof(ConvertToVector(rangeProofJSON, MAX_PROOF_SIZE));
	}

	static RangeProof GetRangeProof(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToRangeProof(GetRequiredField(parentJSON, key));
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
};