#pragma once

#include <Common/Util/HexUtil.h>
#include <Crypto/Models/BlindingFactor.h>
#include <Crypto/Models/Commitment.h>
#include <Crypto/Models/PublicKey.h>
#include <Crypto/Models/Signature.h>
#include <Crypto/Models/RangeProof.h>
#include <Core/Exceptions/DeserializationException.h>
#include <Common/Logger.h>
#include <json/json.h>
#include <sstream>
#include <optional>
#include <vector>

class JsonUtil
{
public:
	static Json::Value Parse(const std::vector<unsigned char>& bytes)
	{
		Json::Value json;
		std::string errors;

		std::string str((const char*)bytes.data(), bytes.size());
		std::stringstream ss(str);
		const bool jsonParsed = Json::parseFromStream(Json::CharReaderBuilder(), ss, &json, &errors);
		if (!jsonParsed)
		{
			LOG_ERROR("Failed to parse json");
			throw DESERIALIZATION_EXCEPTION("Failed to parse json");
		}

		return json;
	}

	static Json::Value ConvertToJSON(const std::vector<unsigned char>& bytes)
	{
		return Json::Value(HexUtil::ConvertToHex(bytes));
	}

	static std::vector<unsigned char> ConvertToVector(const Json::Value& node, const size_t expectedSize)
	{
		if (node == Json::nullValue)
		{
			throw DESERIALIZATION_EXCEPTION("json value was null");
		}

		std::vector<unsigned char> bytes = HexUtil::FromHex(std::string(node.asString()));
		if (bytes.size() != expectedSize)
		{
			throw DESERIALIZATION_EXCEPTION_F("Expected {} bytes, but was {}", expectedSize, bytes.size());
		}

		return bytes;
	}

	static Json::Value GetRequiredField(const Json::Value& node, const std::string& key)
	{
		if (node == Json::nullValue)
		{
			throw DESERIALIZATION_EXCEPTION("json value was null");
		}

		const Json::Value value = node.get(key, Json::nullValue);
		if (value == Json::nullValue)
		{
			throw DESERIALIZATION_EXCEPTION_F("Json value {} was null", key);
		}

		return value;
	}

	static std::optional<Json::Value> GetOptionalField(const Json::Value& node, const std::string& key)
	{
		if (node == Json::nullValue)
		{
			throw DESERIALIZATION_EXCEPTION("json value was null");
		}

		Json::Value value = node.get(key, Json::nullValue);
		if (!value.isNull())
		{
			return std::make_optional(std::move(value));
		}
		else
		{
			return std::nullopt;
		}
	}

	static std::vector<Json::Value> GetRequiredArray(const Json::Value& node, const std::string& key)
	{
		Json::Value value = GetRequiredField(node, key);
		if (!value.isArray())
		{
			throw DESERIALIZATION_EXCEPTION("json value was not an array");
		}

		std::vector<Json::Value> values;
		for (auto iter = value.begin(); iter != value.end(); iter++)
		{
			values.push_back(*iter);
		}

		return values;
	}

	static std::vector<Json::Value> GetArray(const Json::Value& node, const std::string& key)
	{
		std::vector<Json::Value> values;

		std::optional<Json::Value> valueOpt = GetOptionalField(node, key);
		if (valueOpt.has_value() && valueOpt.value().isArray())
		{
			for (auto iter = valueOpt.value().begin(); iter != valueOpt.value().end(); iter++)
			{
				values.push_back(*iter);
			}
		}

		return values;
	}

	//
	// BigInteger
	//
	template<size_t T> 
	static CBigInteger<T> GetBigInteger(const Json::Value& parentJSON, const std::string& key)
	{
		return CBigInteger<T>(ConvertToVector(GetRequiredField(parentJSON, key), T));
	}

	template<size_t T>
	static std::optional<CBigInteger<T>> GetBigIntegerOpt(const Json::Value& parentJSON, const std::string& key)
	{
		std::optional<Json::Value> fieldOpt = GetOptionalField(parentJSON, key);
		if (fieldOpt.has_value()) {
			return std::make_optional(CBigInteger<T>{ ConvertToVector(fieldOpt.value(), T) });
		} else {
			return std::nullopt;
		}
	}

	//
	// BlindingFactors
	//
	static Json::Value ConvertToJSON(const BlindingFactor& blindingFactor)
	{
		return ConvertToJSON(blindingFactor.GetVec());
	}

	static BlindingFactor ConvertToBlindingFactor(const Json::Value& blindingFactorJSON)
	{
		return BlindingFactor(CBigInteger<32>(ConvertToVector(blindingFactorJSON, 32)));
	}

	static BlindingFactor GetBlindingFactor(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToBlindingFactor(GetRequiredField(parentJSON, key));
	}

	static std::optional<BlindingFactor> GetBlindingFactorOpt(const Json::Value& parentJSON, const std::string& key)
	{
		const std::optional<Json::Value> json = JsonUtil::GetOptionalField(parentJSON, key);
		if (!json.has_value())
		{
			return std::nullopt;
		}
		else
		{
			return std::make_optional(ConvertToBlindingFactor(json.value()));
		}
	}

	//
	// Hashes
	//
	static Json::Value ConvertToJSON(const Hash& hash)
	{
		return Json::Value(hash.ToHex());
	}

	static Hash ConvertToHash(const Json::Value& json)
	{
		return Hash(ConvertToVector(json, 32));
	}

	static Hash GetHash(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToHash(GetRequiredField(parentJSON, key));
	}

	//
	// Commitments
	//
	static Json::Value ConvertToJSON(const Commitment& commitment)
	{
		return ConvertToJSON(commitment.GetVec());
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
	// PublicKeys
	//
	static Json::Value ConvertToJSON(const PublicKey& publicKey)
	{
		return ConvertToJSON(publicKey.GetCompressedBytes().GetData());
	}

	static PublicKey ConvertToPublicKey(const Json::Value& publicKeyJSON)
	{
		return PublicKey(CBigInteger<33>(ConvertToVector(publicKeyJSON, 33)));
	}

	static PublicKey GetPublicKey(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToPublicKey(GetRequiredField(parentJSON, key));
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
		return signatureOpt.has_value() ? ConvertToJSON(signatureOpt.value()) : Json::nullValue;
	}

	static std::optional<Signature> ConvertToSignatureOpt(const Json::Value& signatureJSON)
	{
		if (signatureJSON == Json::nullValue)
		{
			return std::nullopt;
		}

		return std::make_optional(CBigInteger<64>(ConvertToVector(signatureJSON, 64)));
	}

	static std::optional<Signature> GetSignatureOpt(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToSignatureOpt(GetOptionalField(parentJSON, key).value_or(Json::Value(Json::nullValue)));
	}

	//
	// CompactSignature
	//
	static CompactSignature ConvertToCompactSignature(const Json::Value& signatureJSON)
	{
		return CompactSignature(CBigInteger<64>(ConvertToVector(signatureJSON, 64)));
	}

	static CompactSignature GetCompactSignature(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToCompactSignature(GetRequiredField(parentJSON, key));
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

	static std::optional<RangeProof> GetRangeProofOpt(const Json::Value& parentJSON, const std::string& key)
	{
		std::optional<Json::Value> jsonOpt = GetOptionalField(parentJSON, key);
		if (!jsonOpt.has_value())
		{
			return std::nullopt;
		}

		return ConvertToRangeProof(jsonOpt.value());
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
		return stringJSON == Json::nullValue ? std::nullopt : std::make_optional(stringJSON.asString());
	}

	static std::optional<std::string> GetStringOpt(const Json::Value& parentJSON, const std::string& key)
	{
		return ConvertToStringOpt(GetOptionalField(parentJSON, key).value_or(Json::Value(Json::nullValue)));
	}

	static std::string GetRequiredString(const Json::Value& parentJSON, const std::string& key)
	{
		return GetRequiredField(parentJSON, key).asString();
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
			throw DESERIALIZATION_EXCEPTION("Value is not a uint64");
		}
	}

	static uint64_t GetRequiredUInt64(const Json::Value& parentJSON, const std::string& key)
	{
		const Json::Value value = JsonUtil::GetRequiredField(parentJSON, key);
		return ConvertToUInt64(value);
	}

	static std::optional<uint64_t> GetUInt64Opt(const Json::Value& parentJSON, const std::string& key)
	{
		const std::optional<Json::Value> json = JsonUtil::GetOptionalField(parentJSON, key);
		if (!json.has_value())
		{
			return std::nullopt;
		}
		else
		{
			return ConvertToUInt64(json.value());
		}
	}

	//
	// UInt32
	//
	static std::optional<uint32_t> GetUInt32Opt(const Json::Value& parentJSON, const std::string& key)
	{
		const std::optional<Json::Value> json = JsonUtil::GetOptionalField(parentJSON, key);
		if (!json.has_value())
		{
			return std::nullopt;
		}
		else
		{
			return std::make_optional((uint32_t)ConvertToUInt64(json.value()));
		}
	}

	static uint32_t GetRequiredUInt32(const Json::Value& parentJSON, const std::string& key)
	{
		const Json::Value value = JsonUtil::GetRequiredField(parentJSON, key);
		return (uint32_t)ConvertToUInt64(value);
	}

	//
	// UInt16
	//
	static uint16_t GetRequiredUInt16(const Json::Value& parentJSON, const std::string& key)
	{
		const Json::Value value = JsonUtil::GetRequiredField(parentJSON, key);
		return (uint16_t)ConvertToUInt64(value);
	}

	//
	// UInt8
	//
	static uint8_t GetRequiredUInt8(const Json::Value& parentJSON, const std::string& key)
	{
		const Json::Value value = JsonUtil::GetRequiredField(parentJSON, key);
		return (uint8_t)ConvertToUInt64(value);
	}

	static std::optional<uint8_t> GetUInt8Opt(const Json::Value& parentJSON, const std::string& key)
	{
		const std::optional<Json::Value> json = JsonUtil::GetOptionalField(parentJSON, key);
		if (!json.has_value())
		{
			return std::nullopt;
		}
		else
		{
			return (uint8_t)ConvertToUInt64(json.value());
		}
	}

	//
	// Int64
	//
	static int64_t ConvertToInt64(const Json::Value& int64JSON)
	{
		if (int64JSON.isInt64())
		{
			return int64JSON.asInt64();
		}
		else if (int64JSON.isString())
		{
			return std::stoll(int64JSON.asString());
		}
		else
		{
			throw DESERIALIZATION_EXCEPTION("Value is not an int64");
		}
	}

	static int64_t GetRequiredInt64(const Json::Value& parentJSON, const std::string& key)
	{
		const Json::Value value = JsonUtil::GetRequiredField(parentJSON, key);
		return ConvertToInt64(value);
	}

	//
	// Bool
	//
	static int64_t ConvertToBool(const Json::Value& boolJson)
	{
		if (boolJson.isBool())
		{
			return boolJson.asBool();
		}
		else if (boolJson.isString())
		{
			auto str = StringUtil::ToLower(boolJson.asString());
			if (str == "true")
			{
				return true;
			}
			else if (str == "false")
			{
				return false;
			}
		}

		throw DESERIALIZATION_EXCEPTION("Value is not a bool");
	}

	static bool GetRequiredBool(const Json::Value& parentJSON, const std::string& key)
	{
		const Json::Value value = JsonUtil::GetRequiredField(parentJSON, key);
		return ConvertToBool(value);
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

	// Write
	static std::string WriteCondensed(const Json::Value& json)
	{
		Json::StreamWriterBuilder builder;
		builder["indentation"] = ""; // Removes whitespaces
		return Json::writeString(builder, json);
	}
};