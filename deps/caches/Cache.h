#pragma once

#include <caches/include/cache.hpp>
#include <caches/include/fifo_cache_policy.hpp>
#include <caches/include/lru_cache_policy.hpp>

template <typename Key, typename Value>
using FIFOCache = typename caches::fixed_sized_cache<Key, Value, caches::FIFOCachePolicy<Key>>;

template <typename Key, typename Value>
using LRUCache = typename caches::fixed_sized_cache<Key, Value, caches::LRUCachePolicy<Key>>;