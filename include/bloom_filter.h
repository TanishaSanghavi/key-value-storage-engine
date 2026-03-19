#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <cstddef>
#include <string>
#include <vector>

class BloomFilter {
public:
    explicit BloomFilter(std::size_t bit_count = 256);
    void add(const std::string& key);
    bool possibly_contains(const std::string& key) const;

private:
    std::size_t hash1(const std::string& key) const;
    std::size_t hash2(const std::string& key) const;
    std::size_t hash3(const std::string& key) const;

    std::vector<bool> bits_;
    std::size_t bit_count_;
};

#endif  // BLOOM_FILTER_H
