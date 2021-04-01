#include <Core/Traits/Serializable.h>
#include <Core/Serialization/Serializer.h>

std::vector<uint8_t> Traits::ISerializable::Serialized(const EProtocolVersion version) const
{
	Serializer serializer(version);
	Serialize(serializer);
	return serializer.GetBytes();
}

std::vector<uint8_t> Traits::ISerializable::SerializeWithIndex(const uint64_t index) const
{
	Serializer serializer;
	serializer.Append<uint64_t>(index);
	Serialize(serializer);
	return serializer.GetBytes();
}