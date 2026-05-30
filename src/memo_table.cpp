#include "memo_table.h"
#include <stdexcept>

MemoTable::MemoTable(bool key80, double max_lf):use80_(key80),max_lf_(max_lf){ rehash(1024); }
void MemoTable::clear(){ std::fill(meta_.begin(), meta_.end(), 0); size_=0; }
void MemoTable::reserve(size_t e){ size_t need=1024; while(static_cast<size_t>(need*max_lf_)<e) need<<=1; if(need>cap_) rehash(need); }

LookupResult MemoTable::find(const PackedKey& k) const {
    uint64_t h=hashPacked(k); size_t i=static_cast<size_t>(h)&mask_; uint8_t f=fp(h);
    while(true){ uint8_t m=meta_[i]; if(!m) return LookupResult::NotFound; if((m&0x7F)==f && key_lo_[i]==k.lo && (!use80_ || key_hi_[i]==k.hi)) return (m&0x80)?LookupResult::FoundP1:LookupResult::FoundP2; i=(i+1)&mask_; }
}
void MemoTable::insert(const PackedKey& k, bool v){ if((size_+1) > static_cast<size_t>(cap_*max_lf_)) rehash(cap_*2); uint64_t h=hashPacked(k); size_t i=static_cast<size_t>(h)&mask_; uint8_t m=mk(h,v); while(true){ if(!meta_[i]){ meta_[i]=m; key_lo_[i]=k.lo; if(use80_) key_hi_[i]=k.hi; ++size_; return;} if(key_lo_[i]==k.lo && (!use80_ || key_hi_[i]==k.hi)){ meta_[i]=m; return;} i=(i+1)&mask_; }}
void MemoTable::rehash(size_t nc){ if((nc&(nc-1))!=0) throw std::runtime_error("capacity must be power of two"); std::vector<uint64_t> olo=std::move(key_lo_); std::vector<uint16_t> ohi=std::move(key_hi_); std::vector<uint8_t> om=std::move(meta_); size_t oc=cap_; key_lo_.assign(nc,0); if(use80_) key_hi_.assign(nc,0); else key_hi_.clear(); meta_.assign(nc,0); cap_=nc; mask_=nc-1; size_=0; for(size_t i=0;i<oc;++i) if(i<om.size()&&om[i]){ PackedKey k{use80_, olo[i], use80_?ohi[i]:0}; insert(k, (om[i]&0x80)!=0);} }
void MemoTable::restoreRaw(size_t cap,size_t sz,std::vector<uint64_t> lo,std::vector<uint16_t> hi,std::vector<uint8_t> m){ cap_=cap; mask_=cap-1; size_=sz; key_lo_=std::move(lo); key_hi_=std::move(hi); meta_=std::move(m);} 
