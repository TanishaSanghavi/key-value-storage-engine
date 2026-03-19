#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "memtable.h"
#include "sstable.h"

#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#endif

int main() {
    MemTable store;
    SSTable sstable("data");
    std::vector<std::string> sstable_files;
    constexpr std::size_t kFlushThreshold = 5;
    constexpr std::size_t kCompactionThreshold = 2;
    std::string line;

    auto extract_timestamp = [](const std::string& file_name) -> long long {
        const std::string prefix = "sstable_";
        const std::string suffix = ".txt";
        if (file_name.find(prefix) != 0) {
            return -1;
        }
        if (file_name.size() <= prefix.size() + suffix.size()) {
            return -1;
        }
        if (file_name.substr(file_name.size() - suffix.size()) != suffix) {
            return -1;
        }

        const std::string number_part =
            file_name.substr(prefix.size(), file_name.size() - prefix.size() - suffix.size());
        char* end_ptr = nullptr;
        const long long timestamp = std::strtoll(number_part.c_str(), &end_ptr, 10);
        if (end_ptr == nullptr || *end_ptr != '\0') {
            return -1;
        }
        return timestamp;
    };

    auto load_existing_sstables = [&sstable_files, &extract_timestamp]() {
        std::vector<std::pair<long long, std::string>> discovered_files;

#ifdef _WIN32
        _finddata_t file_info;
        intptr_t handle = _findfirst("data/sstable_*.txt", &file_info);
        if (handle != -1) {
            do {
                const std::string file_name(file_info.name);
                const long long timestamp = extract_timestamp(file_name);
                if (timestamp >= 0) {
                    discovered_files.push_back(
                        std::make_pair(timestamp, std::string("data/") + file_name));
                }
            } while (_findnext(handle, &file_info) == 0);
            _findclose(handle);
        }
#else
        DIR* dir = opendir("data");
        if (dir != nullptr) {
            while (true) {
                dirent* entry = readdir(dir);
                if (entry == nullptr) {
                    break;
                }

                const std::string file_name(entry->d_name);
                const long long timestamp = extract_timestamp(file_name);
                if (timestamp >= 0) {
                    discovered_files.push_back(
                        std::make_pair(timestamp, std::string("data/") + file_name));
                }
            }
            closedir(dir);
        }
#endif

        std::sort(
            discovered_files.begin(),
            discovered_files.end(),
            [](const std::pair<long long, std::string>& left,
               const std::pair<long long, std::string>& right) {
                if (left.first != right.first) {
                    return left.first < right.first;
                }
                return left.second < right.second;
            });

        sstable_files.clear();
        for (const auto& item : discovered_files) {
            sstable_files.push_back(item.second);
        }
    };

    auto load_sstable_entries =
        [](const std::string& file_path, std::unordered_map<std::string, std::string>& entries) {
            std::ifstream in_file(file_path);
            if (!in_file.is_open()) {
                return;
            }

            std::string row;
            while (std::getline(in_file, row)) {
                const std::size_t separator_pos = row.find(':');
                if (separator_pos == std::string::npos) {
                    continue;
                }

                const std::string key = row.substr(0, separator_pos);
                const std::string value = row.substr(separator_pos + 1);
                entries[key] = value;
            }
        };

    auto compact_sstables = [&sstable, &sstable_files, &load_sstable_entries]() -> bool {
        std::unordered_map<std::string, std::string> merged_entries;
        for (const std::string& file_path : sstable_files) {
            // Oldest first, newest last -> newer values overwrite older ones.
            load_sstable_entries(file_path, merged_entries);
        }

        MemTable merged_memtable;
        for (const auto& entry : merged_entries) {
            merged_memtable.put(entry.first, entry.second);
        }

        std::string compacted_file_path;
        if (!sstable.write(merged_memtable, compacted_file_path)) {
            return false;
        }

        for (const std::string& old_file : sstable_files) {
            std::remove(old_file.c_str());
        }

        sstable_files.clear();
        sstable_files.push_back(compacted_file_path);
        return true;
    };

    load_existing_sstables();

    std::cout << "Simple Key-Value Store\n";
    std::cout << "Commands: PUT key value | GET key | EXIT\n";

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command.empty()) {
            continue;
        }

        if (command == "EXIT") {
            break;
        } else if (command == "PUT") {
            std::string key;
            std::string value;
            iss >> key >> value;

            if (key.empty() || value.empty()) {
                std::cout << "Usage: PUT key value\n";
                continue;
            }

            store.put(key, value);
            if (store.size() >= kFlushThreshold) {
                std::string written_file_path;
                if (sstable.write(store, written_file_path)) {
                    sstable_files.push_back(written_file_path);
                    store.clear();
                    std::cout << "Flushed to " << written_file_path << '\n';

                    if (sstable_files.size() > kCompactionThreshold) {
                        std::cout << "Compaction triggered\n";
                        if (compact_sstables()) {
                            std::cout << "Compaction completed\n";
                        } else {
                            std::cout << "Compaction failed\n";
                        }
                    }
                } else {
                    std::cout << "Flush failed\n";
                }
            }
            std::cout << "OK\n";
        } else if (command == "GET") {
            std::string key;
            iss >> key;

            if (key.empty()) {
                std::cout << "Usage: GET key\n";
                continue;
            }

            std::string value;
            if (store.get(key, value)) {
                std::cout << value << '\n';
                continue;
            }

            bool found_in_sstable = false;
            for (std::size_t i = sstable_files.size(); i > 0; --i) {
                const std::string& file_path = sstable_files[i - 1];
                if (!sstable.might_contain(file_path, key)) {
                    continue;
                }

                sstable.set_file_path(file_path);
                if (sstable.get(key, value)) {
                    found_in_sstable = true;
                    break;
                }
            }

            if (!found_in_sstable) {
                std::cout << "Not found\n";
            } else {
                std::cout << value << '\n';
            }
        } else {
            std::cout << "Unknown command\n";
        }
    }

    return 0;
}