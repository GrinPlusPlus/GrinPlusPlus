#pragma once

#include <Core/Traits/Serializable.h>
#include <rocksdb/slice.h>

#include <string>
#include <memory>

template<typename T,
    typename SFINAE = typename std::enable_if_t<std::is_base_of_v<Traits::ISerializable, T>>>
struct DBEntry
{
    DBEntry(const rocksdb::Slice& _key, std::unique_ptr<const T>&& _item)
        : key(_key), item(std::move(_item)) { }
    DBEntry(const rocksdb::Slice& _key, const T& _item)
        : key(_key), item(std::make_unique<T>(_item)) { }

    rocksdb::Slice key;
    std::unique_ptr<const T> item;

    std::vector<unsigned char> SerializeValue() const
    {
        return item->Serialized();
    }
};