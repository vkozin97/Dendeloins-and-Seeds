#pragma once
#include <cstdint>
#include <string>
#include "memo_table.h"
#pragma pack(push,1)
struct MemoFileHeader {
    char magic[8]; uint32_t file_version; uint8_t n,k,rules_version,direction_scheme,key_format,canonical_keys,db_scope,complete; uint64_t rules_hash,hash_seed; uint32_t hash_version; uint64_t bucket_capacity,entries_count; uint8_t root_result; uint8_t reserved[64];
};
#pragma pack(pop)

bool saveMemoFile(const std::string& path, const MemoFileHeader& hdr, const MemoTable& t);
bool loadMemoFile(const std::string& path, MemoFileHeader& hdr, MemoTable& t, std::string& err);
