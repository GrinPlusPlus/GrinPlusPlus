#pragma once

#include <Crypto/Crypto.h>
#include <Crypto/Models/PublicKey.h>
#include <Crypto/Models/Signature.h>
#include <Core/Util/JsonUtil.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <optional>

/*
Field           type	                len     notes
complete flag	u8                      1	    If non-zero, entry contains part
xs	            secp256k1 Public Key	33      
nonce           secp256k1 Public Key	33
part	        secp256k1 AggSig	    (64)	If present
*/
struct SlateSignature
{
    SlateSignature() = default;
    SlateSignature(const PublicKey& _excess, const PublicKey& _nonce, const std::optional<CompactSignature>& _partialOpt)
        : excess(_excess), nonce(_nonce), partialOpt(_partialOpt) { }
    PublicKey excess;
    PublicKey nonce;
    std::optional<CompactSignature> partialOpt;

    bool operator==(const SlateSignature& rhs) const noexcept
    {
        return excess == rhs.excess && nonce == rhs.nonce && partialOpt == rhs.partialOpt;
    }

    void Serialize(Serializer& serializer) const noexcept
    {
        serializer.Append<uint8_t>(partialOpt.has_value() ? 1 : 0);
        excess.Serialize(serializer);
        nonce.Serialize(serializer);
        if (partialOpt.has_value()) {
            Crypto::ParseCompactSignature(partialOpt.value()).Serialize(serializer);
        }
    }

    static SlateSignature Deserialize(ByteBuffer& byteBuffer)
    {
        const uint8_t hasSig = byteBuffer.ReadU8();

        SlateSignature ret;
        ret.excess = PublicKey::Deserialize(byteBuffer);
        ret.nonce = PublicKey::Deserialize(byteBuffer);
        if (hasSig > 0) {
            Signature signature = Signature::Deserialize(byteBuffer);
            ret.partialOpt = std::make_optional<CompactSignature>(Crypto::ToCompact(signature));
        }

        return ret;
    }

    Json::Value ToJSON() const noexcept
    {
        Json::Value json;
        json["xs"] = excess.ToHex();
        json["nonce"] = nonce.ToHex();

        if (partialOpt.has_value()) {
            json["part"] = partialOpt.value().ToHex();
        }

        return json;
    }

    static SlateSignature FromJSON(const Json::Value& json)
    {
        SlateSignature sig;
        sig.excess = JsonUtil::GetPublicKey(json, "xs");
        sig.nonce = JsonUtil::GetPublicKey(json, "nonce");
        auto sigOpt = JsonUtil::GetBigIntegerOpt<64>(json, "part");
        if (sigOpt.has_value()) {
            sig.partialOpt = std::make_optional<CompactSignature>(sigOpt.value());
        }
        return sig;
    }
};