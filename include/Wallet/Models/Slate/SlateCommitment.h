#pragma once

#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Models/Features.h>
#include <Crypto/Models/RangeProof.h>
#include <Core/Util/JsonUtil.h>
#include <optional>

/*
Field	            type	        len	    notes
output flag	        u8	            1	    If non-zero, entry is output and contains p (proof)
f	                u8	            1	    features (1 = Coinbase, 0 = Plain)
c	                Commitment	    33
p	                Rangeproof	    675	    If present
*/
struct SlateCommitment
{
    EOutputFeatures features;
    Commitment commitment;
    std::optional<RangeProof> proofOpt;

    bool operator==(const SlateCommitment& rhs) const noexcept
    {
        return features == rhs.features && commitment == rhs.commitment && proofOpt == rhs.proofOpt;
    }

    void Serialize(Serializer& serializer) const noexcept
    {
        serializer.Append<uint8_t>(proofOpt.has_value() ? 1 : 0);
        serializer.Append<uint8_t>((uint8_t)features);
        commitment.Serialize(serializer);

        if (proofOpt.has_value()) {
            proofOpt.value().Serialize(serializer);
        }
    }

    static SlateCommitment Deserialize(ByteBuffer& byteBuffer)
    {
        const uint8_t hasProof = byteBuffer.ReadU8();

        SlateCommitment ret;
        ret.features = (EOutputFeatures)byteBuffer.ReadU8();
        ret.commitment = Commitment::Deserialize(byteBuffer);

        if (hasProof > 0) {
            ret.proofOpt = std::make_optional<RangeProof>(RangeProof::Deserialize(byteBuffer));
        }

        return ret;
    }

    Json::Value ToJSON() const noexcept
    {
        Json::Value json;
        json["c"] = commitment.ToHex();
        
        if (features == EOutputFeatures::COINBASE_OUTPUT)
        {
            json["f"] = 1;
        }

        if (proofOpt.has_value())
        {
            json["p"] = proofOpt.value().ToHex();
        }

        return json;
    }

    static SlateCommitment FromJSON(const Json::Value& json)
    {
        SlateCommitment slateCommitment;
        slateCommitment.commitment = JsonUtil::GetCommitment(json, "c");
        slateCommitment.features = ConvertFeature(JsonUtil::GetUInt64Opt(json, "f").value_or(0));
        slateCommitment.proofOpt = JsonUtil::GetRangeProofOpt(json, "p");
        return slateCommitment;
    }

private:
    static EOutputFeatures ConvertFeature(const uint64_t value)
    {
        return value == 1 ? EOutputFeatures::COINBASE_OUTPUT : EOutputFeatures::DEFAULT;
    }
};