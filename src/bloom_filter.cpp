#include "bloom_filter.h"

#include <functional>

BloomFilter::BloomFilter(std::size_t bit_count)
    : bits_(bit_count, false), bit_count_(bit_count) {}

void BloomFilter::add(const std::string& key) {
    bits_[hash1(key)] = true;
    bits_[hash2(key)] = true;
    bits_[hash3(key)] = true;
}

bool BloomFilter::possibly_contains(const std::string& key) const {
    return bits_[hash1(key)] && bits_[hash2(key)] && bits_[hash3(key)];
}

std::size_t BloomFilter::hash1(const std::string& key) const {
    std::size_t hash = 5381;
    for (char c : key) {
        hash = ((hash << 5) + hash) + static_cast<std::size_t>(c);
    }
    return hash % bit_count_;
}

std::size_t BloomFilter::hash2(const std::string& key) const {
    std::size_t hash = 0;
    for (char c : key) {
        hash = hash * 131 + static_cast<std::size_t>(c);
    }
    return hash % bit_count_;
}

std::size_t BloomFilter::hash3(const std::string& key) const {
    const std::size_t hash = std::hash<std::string>{}(key) ^ 0x9e3779b97f4a7c15ULL;
    return hash % bit_count_;
}
