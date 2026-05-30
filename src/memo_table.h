#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "memo_key.h"

enum class LookupResult { NotFound, FoundP1, FoundP2 };

class MemoTable {
public:
    explicit MemoTable(bool key80=false, double max_lf=0.75);
    LookupResult find(const PackedKey& key) const;
    void insert(const PackedKey& key, bool first_wins);
    void reserve(size_t expected_entries);
    void clear();
    size_t size() const { return size_; }
    size_t capacity() const { return cap_; }
    double loadFactor() const { return cap_? static_cast<double>(size_)/static_cast<double>(cap_) : 0.0; }

    bool key80() const { return use80_; }
    const std::vector<uint64_t>& keyLo() const { return key_lo_; }
    const std::vector<uint16_t>& keyHi() const { return key_hi_; }
    const std::vector<uint8_t>& meta() const { return meta_; }
    void restoreRaw(size_t cap, size_t sz, std::vector<uint64_t> lo, std::vector<uint16_t> hi, std::vector<uint8_t> m);
private:
    void rehash(size_t new_cap);
    static uint8_t fp(uint64_t h){ uint8_t f=static_cast<uint8_t>((h>>57)&0x7F); return f?f:1; }
    static uint8_t mk(uint64_t h,bool v){ return static_cast<uint8_t>(fp(h)|(v?0x80:0)); }
    bool use80_; double max_lf_; size_t cap_=0, mask_=0, size_=0;
    std::vector<uint64_t> key_lo_; std::vector<uint16_t> key_hi_; std::vector<uint8_t> meta_;
};
