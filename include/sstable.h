#ifndef SSTABLE_H
#define SSTABLE_H

#include <string>
#include <unordered_map>

#include "bloom_filter.h"
#include "memtable.h"

class SSTable {
public:
    explicit SSTable(std::string data_directory = "data");
    bool write(const MemTable& memtable, std::string& written_file_path);
    void set_file_path(const std::string& file_path);
    bool get(const std::string& key, std::string& value) const;
    bool might_contain(const std::string& file_path, const std::string& key) const;

private:
    std::string data_directory_;
    std::string file_path_;
    std::unordered_map<std::string, BloomFilter> bloom_filters_;
    std::string build_file_path() const;
};

#endif  // SSTABLE_H
