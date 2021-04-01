#pragma once

#include <Common/Secure.h>
#include <Core/Serialization/EndianHelper.h>
#include <Core/Traits/Serializable.h>
#include <Core/Enums/ProtocolVersion.h>
#include <Crypto/Models/BigInteger.h>

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

enum class ESerializeLength
{
    U32,
    U64,
    NONE
};

class Serializer
{
public:
    Serializer(const EProtocolVersion protocolVersion = EProtocolVersion::V1)
        : m_protocolVersion(protocolVersion)
    {
    }
    Serializer(const size_t expectedSize, const EProtocolVersion protocolVersion = EProtocolVersion::V1)
        : m_protocolVersion(protocolVersion)
    {
        m_serialized.reserve(expectedSize);
    }

    template <class T, typename SFINAE = typename std::enable_if_t<std::is_integral_v<T>>>
    Serializer& Append(const T& t)
    {
        std::vector<uint8_t> temp(sizeof(T));
        memcpy(&temp[0], &t, sizeof(T));

        if (EndianHelper::IsBigEndian()) {
            m_serialized.insert(m_serialized.end(), temp.cbegin(), temp.cend());
        } else {
            m_serialized.insert(m_serialized.end(), temp.crbegin(), temp.crend());
        }

        return *this;
    }

    template <class T>
    Serializer& AppendLittleEndian(const T& t)
    {
        std::vector<uint8_t> temp(sizeof(T));
        memcpy(&temp[0], &t, sizeof(T));

        if (EndianHelper::IsBigEndian()) {
            m_serialized.insert(m_serialized.end(), temp.crbegin(), temp.crend());
        } else {
            m_serialized.insert(m_serialized.end(), temp.cbegin(), temp.cend());
        }

        return *this;
    }

    template <size_t T>
    Serializer& Append(const std::array<uint8_t, T>& arr)
    {
        m_serialized.insert(m_serialized.end(), arr.cbegin(), arr.cend());
        return *this;
    }

    Serializer& Append(const Traits::ISerializable& serializable)
    {
        serializable.Serialize(*this);
        return *this;
    }

    Serializer& Append(const std::shared_ptr<const Traits::ISerializable>& pSerializable)
    {
        pSerializable->Serialize(*this);
        return *this;
    }

    Serializer& Append(const std::vector<uint8_t>& bytes, const ESerializeLength prepend_length = ESerializeLength::NONE)
    {
        AppendLength(prepend_length, bytes.size());
        m_serialized.insert(m_serialized.end(), bytes.cbegin(), bytes.cend());
        return *this;
    }

    Serializer& AppendByteVector(const std::vector<uint8_t>& bytes, const ESerializeLength prepend_length = ESerializeLength::NONE)
    {
        AppendLength(prepend_length, bytes.size());
        m_serialized.insert(m_serialized.end(), bytes.cbegin(), bytes.cend());
        return *this;
    }

    Serializer& AppendByteVector(const SecureVector& bytes, const ESerializeLength prepend_length = ESerializeLength::NONE)
    {
        AppendLength(prepend_length, bytes.size());
        m_serialized.insert(m_serialized.end(), bytes.cbegin(), bytes.cend());
        return *this;
    }

    Serializer& AppendVarStr(const std::string& varString)
    {
        AppendLength(ESerializeLength::U64, varString.length());
        m_serialized.insert(m_serialized.end(), varString.cbegin(), varString.cend());
        return *this;
    }

    Serializer& AppendStr(const std::string& str, const ESerializeLength prepend_length = ESerializeLength::NONE)
    {
        AppendLength(prepend_length, str.length());
        m_serialized.insert(m_serialized.end(), str.cbegin(), str.cend());
        return *this;
    }

    template<size_t NUM_BYTES>
    Serializer& AppendBigInteger(const CBigInteger<NUM_BYTES>& bigInteger)
    {
        m_serialized.insert(m_serialized.end(), bigInteger.cbegin(), bigInteger.cend());
        return *this;
    }

    const std::vector<uint8_t>& GetBytes() const noexcept { return m_serialized; }
    EProtocolVersion GetProtocolVersion() const noexcept { return m_protocolVersion; }

    const std::vector<uint8_t>& vec() const noexcept { return m_serialized; }
    const uint8_t* data() const noexcept { return m_serialized.data(); }
    size_t size() const noexcept { return m_serialized.size(); }

    auto begin() noexcept { return m_serialized.begin(); }
    auto end() noexcept { return m_serialized.end(); }

    auto cbegin() const noexcept { return m_serialized.cbegin(); }
    auto cend() const noexcept { return m_serialized.cend(); }

    // WARNING: This will destroy the contents of m_serialized.
    // TODO: Create a SecureSerializer instead.
    SecureVector GetSecureBytes()
    {
        SecureVector secureBytes(m_serialized.begin(), m_serialized.end());
        SecureMem::Cleanse(m_serialized.data(), m_serialized.size());

        return secureBytes;
    }

private:
    void AppendLength(const ESerializeLength prepend_length, const size_t length)
    {
        if (prepend_length == ESerializeLength::U64) {
            Append<uint64_t>(length);
        } else if (prepend_length == ESerializeLength::U32) {
            Append<uint32_t>((uint32_t)length);
        }
    }

    EProtocolVersion m_protocolVersion;
    std::vector<uint8_t> m_serialized;
};