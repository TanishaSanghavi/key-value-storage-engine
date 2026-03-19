# High-Performance LSM-Tree Key-Value Storage Engine (C++)

A high-performance key-value storage engine built in C++, inspired by LSM-tree systems like LevelDB and RocksDB.

## 📌 Overview
- Supports: PUT, GET
- Architecture:
  - MemTable (in-memory writes)
  - SSTables (disk persistence)
  - Bloom Filters (read optimization)
  - Compaction (merge & optimize storage)

---

## Features

### MemTable
- In-memory storage using unordered_map
- Fast writes and reads
- Flush threshold: 5 entries

### SSTables
- Immutable sorted files on disk
- Format: key:value
- Stored in data/
- Naming: sstable_<timestamp>.txt

### Bloom Filters
- One per SSTable
- Skips unnecessary disk reads
- No false negatives

### Compaction
- Triggered when SSTables > 2
- Merges files, keeps latest values
- Reduces read amplification

---

## Architecture

Client → MemTable → SSTables → Bloom Filters → Compaction

---

## Data Flow

Write (PUT):
- Insert into MemTable
- Flush to SSTable on threshold
- Trigger compaction if needed

Read (GET):
- Check MemTable
- Check SSTables (newest → oldest)
- Use Bloom filter before disk read

---

## Example

PUT user1 Alice  
GET user1 → Alice  
GET unknown → Not found  

---

## Performance
- Write-optimized (sequential disk writes)
- Read-optimized (Bloom filters)
- Reduced read amplification via compaction

---

## Build & Run

Windows:
g++ -std=c++17 -Iinclude src/main.cpp src/memtable.cpp src/sstable.cpp src/bloom_filter.cpp -o kv_store.exe  
.\kv_store.exe  

Linux / macOS:
g++ -std=c++17 -Iinclude src/main.cpp src/memtable.cpp src/sstable.cpp src/bloom_filter.cpp -o kv_store  
./kv_store  
