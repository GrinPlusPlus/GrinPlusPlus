#pragma once

#include <Core/Traits/Serializable.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Util/JsonUtil.h>
#include <json/json.h>

class Fee : public Traits::ISerializable
{
public:
	//
	// Constructors
	//
	Fee() : m_shift(0), m_fee(0) { }
	Fee(const Fee& fee) = default;
	Fee(Fee&& fee) noexcept = default;
	Fee(const uint8_t shift, const uint64_t fee)
		: m_shift(shift), m_fee(fee) { }
	
	static Fee From(const uint64_t raw_fee) { return Fee(0, raw_fee); }

	//
	// Operators
	//
	Fee& operator=(const Fee& fee) = default;
	Fee& operator=(Fee&& fee) noexcept = default;
	bool operator==(const Fee& rhs) const { return m_fee == rhs.m_fee && m_shift == rhs.m_shift; }
	bool operator!=(const Fee& rhs) const { return m_fee != rhs.m_fee || m_shift != rhs.m_shift; }

	//
	// Getters
	//
	uint8_t GetShift() const noexcept { return m_shift; }
	uint64_t GetFee() const noexcept { return m_fee; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const final
	{
		serializer.Append<uint16_t>(0);
		serializer.Append<uint8_t>(m_shift);
		serializer.Append<uint8_t>((uint8_t)(m_fee >> 32));
		serializer.Append<uint32_t>((uint32_t)(m_fee & 0xffffffff));
	}

	static Fee Deserialize(ByteBuffer& byteBuffer)
	{
		byteBuffer.ReadU16();
		uint8_t shift = byteBuffer.ReadU8() & 0x0f;
		uint64_t fee = ((uint64_t)byteBuffer.ReadU8()) << 32;
		fee += byteBuffer.ReadU32();
		return Fee(shift, fee);
	}

	Json::Value ToJSON() const
	{
		Json::Value json;
		json["shift"] = m_shift;
		json["fee"] = m_fee;
		return json;
	}

	static Fee FromJSON(const Json::Value& json)
	{
		if (json.isObject()) {
			uint8_t shift = JsonUtil::GetRequiredUInt8(json, "shift");
			uint64_t fee = JsonUtil::GetRequiredUInt64(json, "fee");
			return Fee(shift, fee);
		} else {
			uint64_t fee = json.asUInt64();
			return Fee::From(fee);
		}
	}

private:
	uint8_t m_shift;
	uint64_t m_fee;
};