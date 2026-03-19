#include "sstable.h"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace {
bool ensure_directory_exists(const std::string& path) {
#ifdef _WIN32
    const int result = _mkdir(path.c_str());
#else
    const int result = mkdir(path.c_str(), 0755);
#endif
    if (result == 0) {
        return true;
    }

    // EEXIST means the directory already exists.
    return errno == EEXIST;
}
}  // namespace

SSTable::SSTable(std::string data_directory)
    : data_directory_(std::move(data_directory)) {}

bool SSTable::write(const MemTable& memtable, std::string& written_file_path) {
    if (!ensure_directory_exists(data_directory_)) {
        return false;
    }

    written_file_path = build_file_path();

    std::vector<std::pair<std::string, std::string>> entries(
        memtable.entries().begin(), memtable.entries().end());

    std::sort(
        entries.begin(),
        entries.end(),
        [](const auto& left, const auto& right) { return left.first < right.first; });

    std::ofstream out_file(written_file_path);
    if (!out_file.is_open()) {
        return false;
    }

    for (const auto& entry : entries) {
        out_file << entry.first << ":" << entry.second << '\n';
    }

    BloomFilter bloom_filter;
    for (const auto& entry : entries) {
        bloom_filter.add(entry.first);
    }
    bloom_filters_[written_file_path] = bloom_filter;

    return true;
}

void SSTable::set_file_path(const std::string& file_path) {
    file_path_ = file_path;
}

bool SSTable::get(const std::string& key, std::string& value) const {
    if (file_path_.empty()) {
        return false;
    }

    std::ifstream in_file(file_path_);
    if (!in_file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(in_file, line)) {
        const std::size_t separator_pos = line.find(':');
        if (separator_pos == std::string::npos) {
            continue;
        }

        const std::string current_key = line.substr(0, separator_pos);
        if (current_key != key) {
            continue;
        }

        value = line.substr(separator_pos + 1);
        return true;
    }

    return false;
}

bool SSTable::might_contain(const std::string& file_path, const std::string& key) const {
    const auto it = bloom_filters_.find(file_path);
    if (it == bloom_filters_.end()) {
        // If no filter exists, fall back to reading the file.
        return true;
    }

    return it->second.possibly_contains(key);
}

std::string SSTable::build_file_path() const {
    const auto now = std::chrono::system_clock::now();
    const auto timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    return data_directory_ + "/sstable_" + std::to_string(timestamp) + ".txt";
}
