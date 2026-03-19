#include "memtable.h"

void MemTable::put(const std::string& key, const std::string& value) {
    table_[key] = value;
}

bool MemTable::get(const std::string& key, std::string& value) const {
    const auto it = table_.find(key);
    if (it == table_.end()) {
        return false;
    }

    value = it->second;
    return true;
}

std::size_t MemTable::size() const {
    return table_.size();
}

void MemTable::clear() {
    table_.clear();
}

const std::unordered_map<std::string, std::string>& MemTable::entries() const {
    return table_;
}
