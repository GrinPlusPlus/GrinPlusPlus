#pragma once

#include <cppcodec/base64_rfc4648.hpp>

namespace cppcodec {

    namespace detail {

        class base64_rfc4648_unpadded : public base64_rfc4648
        {
        public:
            template <typename Codec> using codec_impl = stream_codec<Codec, base64_rfc4648_unpadded>;

            static CPPCODEC_ALWAYS_INLINE constexpr bool generates_padding() { return false; }
            static CPPCODEC_ALWAYS_INLINE constexpr bool requires_padding() { return false; }
        };

    } // namespace detail

    using base64_rfc4648_unpadded = detail::codec<detail::base64<detail::base64_rfc4648_unpadded>>;

} // namespace cppcodec

class Base64
{
public:
    static std::string EncodeUnpadded(const std::vector<uint8_t>& data)
    {
        return cppcodec::base64_rfc4648_unpadded::encode(data);
    }

    static std::vector<uint8_t> DecodeUnpadded(const std::string& encoded)
    {
        return cppcodec::base64_rfc4648_unpadded::decode(encoded);
    }
};