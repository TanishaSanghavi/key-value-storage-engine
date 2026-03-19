#ifndef MEMTABLE_H
#define MEMTABLE_H

#include <cstddef>
#include <string>
#include <unordered_map>

class MemTable {
public:
    void put(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value) const;
    std::size_t size() const;
    void clear();
    const std::unordered_map<std::string, std::string>& entries() const;

private:
    std::unordered_map<std::string, std::string> table_;
};

#endif  // MEMTABLE_H
