#pragma once

#include <set>
#include "common_defs.h"
#include "scans.h"
#include "literal.h"
#include "compare.h"
#include <cassert>
#include <ostream>
#include <map>

#include <iostream>

struct AndCmp {
    u64 and_mask;
    u64 cmp_mask;

    AndCmp() : and_mask(0), cmp_mask(0) {
    }

    bool care(u32 bit) const {
        return (1ULL<<bit) & and_mask;
    }

    bool one(u32 bit) const {
        assert(care(bit));
        return (1ULL<<bit) & cmp_mask;
    }

    bool operator<(const AndCmp & o) const {
        if (and_mask != o.and_mask) {
            return and_mask < o.and_mask;
        }
        return cmp_mask < o.cmp_mask;
    }

    void set_one(u32 bit) {
        and_mask |= 1ULL<<bit;
        cmp_mask |= 1ULL<<bit;
    }

    void set_zero(u32 bit) {
        and_mask |= 1ULL<<bit;
        cmp_mask &= ~(1ULL<<bit);
    }

    AndCmp pext(u64 pext_mask) const {
        AndCmp n;
        n.and_mask = _pext_u64(and_mask, pext_mask);
        n.cmp_mask = _pext_u64(cmp_mask, pext_mask);
        return n;
    }
};

std::ostream & operator<<(std::ostream & os, const AndCmp & ac) {
    for (u32 i = 0; i < 64; i++) {
        if (ac.care(i)) {
            os << (ac.one(i) ? '1' : '0');
        } else {
            os << '.';
        }
    }
    return os;
}

// an exotic: PEXT an AndCmp to yield another AndCmp

// another exotic: iterator over an AndCmp to retrieve all possible values
// needs to be given a number of significant bits


class NVS {
    std::vector<Literal> lits; // all the actual literals are here and are gotten at by index everywhere else (u32)
    std::vector<std::vector<u32>> index_tables; // table 0 here is always empty and isn't reached
    u32 * primary_table;
    //std::vector<u32> offsets_to_index_tables; // these are the offsets from the pext result to the index_tables
    u64 pext_mask;                          

    std::vector<AndCmp> and_cmps;

    typedef std::map<std::set<u32>, std::set<AndCmp>> Construction;

    void dump_construction(std::ostream & os, Construction & c) {
        for (auto i : c) {
            os << "{";
            copy(i.first.begin(), i.first.end(), std::ostream_iterator<u32>(os, ","));
            os << "} : ";
            for (auto j : i.second) {
                os << j << " ";
            }
            os << "\n";
        }
    }

    std::tuple<u32, u32, u32> count_lits_to_one_zero_dc(std::set<u32> s, u32 bit) {
        u32 dc_count = 0, zero_count = 0, one_count = 0;
        for (u32 idx : s) {
            if (and_cmps[idx].care(bit)) {
                if (and_cmps[idx].one(bit)) {
                    one_count++;
                } else {
                    zero_count++;
                }
            } else {
                dc_count++;
            }
        }
        return std::make_tuple(one_count, zero_count, dc_count);
    }

    // given a construction and a count of bits, find the best bits to 
    // split on given that we've already used the bits in used_bits
    // return at most num_bits, but can return as few as 0 results
    // (thus an empty vector) if nothing is that great
    // Initial cut will just score our bits globally and return a 'good enough' single chunk of bits
    // there are almost certainly faster ways of doing the split if we know a bunch of bits at once
    // however, there's a big space to explore and picking one bit at a time with a goal of making
    // the best split is pretty good.
    std::vector<u32> find_good_bits(UNUSED Construction & c, u32 num_bits, std::set<u32> & used_bits) {
        std::vector<u32> good_bits;
        // debugging placeholder
        for (u32 bit = 63; bit > 0 && good_bits.size() < num_bits; bit--) {
            if (used_bits.find(bit) == used_bits.end()) {
                good_bits.push_back(bit);
            }
        }
        return good_bits;
    }

    void split_on_bit(Construction & c, u32 bit) {
        Construction tmp;

        for (auto & i : c) {
            // split i.first (our set of indices) into two sets
            // if any elements have different values on this bit
            // otherwise leave alone and just put the set and AndCmp values through as is
            assert(!i.first.empty());
            std::set<u32> zero_set, one_set;

            for (u32 idx : i.first) {
                if (and_cmps[idx].care(bit)) {
                    if (and_cmps[idx].one(bit)) {
//                        std::cout << "Idx: " << idx << "->1\n";
                        one_set.insert(idx);
                    } else {
//                        std::cout << "Idx: " << idx << "->0\n";
                        zero_set.insert(idx);
                    }
                } else {
//                    std::cout << "Idx: " << idx << "->0,1\n";
                    one_set.insert(idx);
                    zero_set.insert(idx);
                }
            }
            std::set<AndCmp> new_zero_masks, new_one_masks;
            for (auto j : i.second) {
                j.set_zero(bit);
                new_zero_masks.insert(j);
                j.set_one(bit);
                new_one_masks.insert(j);
            }
            if (!one_set.empty()) {
                tmp[one_set].merge(new_one_masks);
            }
            if (!zero_set.empty()) {
                tmp[zero_set].merge(new_zero_masks);
            }
        }
        c.swap(tmp);
    }

    // turn this construction into the actual bytecode (current a bunch of STL things
    // but will make into a real bytecode eventually)
    void construction_to_bytecode(UNUSED Construction & c, std::set<u32> & used_bits) {
        using namespace std;
        // fill in PEXT mask based on used bits
        for (auto i : used_bits) {
            pext_mask |= 1ULL << i;
        }

        // make a mask of 1s for all unused high bits. We will put this into our 
        // AND mask to force a 'care zero' value for things we aren't PEXT'ing on
        u64 unused_bits_high_mask = -1;
        unused_bits_high_mask <<= used_bits.size();

        index_tables.resize(c.size()+1); // need one index table extra as zero is reserved
        primary_table = new u32[1 << used_bits.size()];
        memset(primary_table, 0, (1 << used_bits.size()) * sizeof(u32));
        //offsets_to_index_tables.resize(1 << used_bits.size());
        
        // for each entry in our construction
        u32 current_index_table = 1; // can't use zero
        for (auto & i : c) {
            // make an index table for this starting at index 1 comprising all our lit ids
            copy(i.first.begin(), i.first.end(), back_inserter(index_tables[current_index_table]));

            cout << "Putting these strings: ";
            for (auto & j : i.first) {
                cout << j << " ";
            }
            cout << "into index table " << current_index_table << "\n";
            // then iterate through our AndCmp masks after PEXT-ing them both into
            // PEXTed-AndCmp - this gives us something we iterate over. Every entry for
            // this needs to point to our index table. We should never encounter the
            // same table value twice, so iterate everything to 0.
            for (auto & j : i.second) {
                AndCmp tmp = j.pext(pext_mask);
                u64 and_mask = tmp.and_mask | unused_bits_high_mask;
                u64 cmp_mask = tmp.cmp_mask;
                // now iterate over the don't care bits in c
                u64 v = and_mask;
                u64 addend = (~and_mask & -~and_mask); // lowest clear bit in and_mask
 //               std::cout << " and_mask: " << and_mask << "\n";
 //               std::cout << " cmp_mask: " << cmp_mask << "\n";
                do {
                    u64 val = (cmp_mask & and_mask) | (v & (~and_mask));
                    assert(primary_table[val] == 0);
                    primary_table[val] = current_index_table;
                    cout << "table entry " << val << " points to current_index_table " << current_index_table << "\n";
//                    std::cout << " val: " << val << "\n";
                    v = (v + addend) | and_mask;
//                    std::cout << "v: " << v << "\n";
                
                } while (v != and_mask);
            }
            current_index_table++;
        }
    }

public:
    typedef OffsetIDResult ResultType;

    NVS(const std::vector<Literal> & lits_in) : lits(lits_in) {
        using namespace std;
        // make a bullshit version of this with everything in one table
        // and a PEXT mask of 0 to start with for testing
        pext_mask = 0;
        and_cmps.resize(lits.size());

        for (u32 i = 0; i < lits.size(); i++) {
            std::cout << "Literal " << i << " is " << lits[i].s << "\n";
            std::pair<u64, u64> cm = lits[i].toFinalCmpMsk();
            and_cmps[i].cmp_mask = cm.first; 
            and_cmps[i].and_mask = cm.second; 
        }

        Construction c;
        set<u32> start;
        for (u32 i = 0; i < lits.size(); i++) {
            start.insert(i);
        }
        set<AndCmp> ac_set;
        ac_set.insert(AndCmp());
        c[start] = ac_set;

        dump_construction(std::cout, c);

        set<u32> used_bits;
        const size_t BIT_LIMIT = 8;

        while (used_bits.size() < BIT_LIMIT) {
            vector<u32> good_bits = find_good_bits(c, 1, used_bits);
            if (good_bits.empty())
                break;
            for (auto b : good_bits) {
//                cout << " Splitting on " << b << "\n";
                used_bits.insert(b);
                split_on_bit(c, b);
                dump_construction(std::cout, c);
            }
        }
        construction_to_bytecode(c, used_bits);
    }

    inline void handle_table(const InputBlock input, u32 table, size_t i, Result<ResultType> & out, u32 & result_idx) const {
        for (auto & li : index_tables[table]) {
            auto & l = lits[li];
            if (l.compare_in(input, i)) {
                out.results[result_idx++] = std::make_pair(i, l.id);
            }
        }
    }

    void scan(const InputBlock input, Result<ResultType> & out) const {
        u32 result_idx = 0;
        size_t i = input.start;
        
        u64 pm = pext_mask;
        u64 v_start = 0;
        for (; i < (input.start + 7) && (i < input.end); i++) {
            v_start |= input.buf[i]; 
            u64 idx = _pext_u64(v_start, pm);
            u32 table = primary_table[idx];
            if (unlikely(table)) {
                handle_table(input, table, i, out, result_idx);
            }
            v_start <<= 8;
        }

        // strangely, we get a better result from manually inlining the first couple handle_table calls
        // Problems
        // 1. We are pulling our PEXT masks from memory for some reason. The compiler seems reluctant to 
        //    keep them in a register. It's possible that the nested literal compare loop smashes so many
        //    registers that things are awkward. I'm not sure. 

        // 2. We should adapt our PEXT masks to allow multiple uses of the same input. This can be achieved 
        //    by disallowing use of the earliest 1 byte (if we want to reuse input twice) or 3 bytes (if we 
        //    want to reuse input four times). This cuts our loads, although the perverse habit of the compiler
        //    w.r.t. pulling PEXT masks from memory makes this even more frustrating
        
        // 3. We need to investigate whether putting our PEXT results into a buffer and reading a pair of i, 
        //    table is faster. I think this always winds up being a bust to be honest.

        // 4. We should have a real run-time on the other end of this. The fact that we go off annd iterate over
        //    a C++ vector is probably asking for trouble. Similar loops in the past haven't displayed such weird
        //    behavior so you likely need to bite the bullet and do this properly.

        for (; i+5 < input.end; i+=6) {
            u64 v0 = *(u64 *)&input.buf[i-7];
            u64 idx_0 = _pext_u64(v0, pm);
            u32 table_0 = primary_table[idx_0];

            u64 v1 = *(u64 *)&input.buf[i-6];
            u64 idx_1 = _pext_u64(v1, pm);
            u32 table_1 = primary_table[idx_1];

            if (unlikely(table_0)) {
                for (auto & li : index_tables[table_0]) {
                    auto & l = lits[li];
                    if (l.compare_in(input, i)) {
                        out.results[result_idx++] = std::make_pair(i, l.id);
                    }
                }
                /*
                handle_table(input, table_0, i, out, result_idx);
                */
            }
            if (unlikely(table_1)) {
                for (auto & li : index_tables[table_1]) {
                    auto & l = lits[li];
                    if (l.compare_in(input, i+1)) {
                        out.results[result_idx++] = std::make_pair(i+1, l.id);
                    }
                }
                //handle_table(input, table_1, i+1, out, result_idx);
            }
            
            u64 v2 = *(u64 *)&input.buf[i-5];
            u64 idx_2 = _pext_u64(v2, pm);
            u32 table_2 = primary_table[idx_2];

            u64 v3 = *(u64 *)&input.buf[i-4];
            u64 idx_3 = _pext_u64(v3, pm);
            u32 table_3 = primary_table[idx_3];

            if (unlikely(table_2)) {
                handle_table(input, table_2, i+2, out, result_idx);
            }
            if (unlikely(table_3)) {
                handle_table(input, table_3, i+3, out, result_idx);
            }

            u64 v4 = *(u64 *)&input.buf[i-3];
            u64 idx_4 = _pext_u64(v4, pm);
            u32 table_4 = primary_table[idx_4];

            u64 v5 = *(u64 *)&input.buf[i-2];
            u64 idx_5 = _pext_u64(v5, pm);
            u32 table_5 = primary_table[idx_5];

            if (unlikely(table_4)) {
                handle_table(input, table_4, i+4, out, result_idx);
            }
            if (unlikely(table_5)) {
                handle_table(input, table_5, i+5, out, result_idx);
            }

        }
        for (; i < input.end; i++) {
            u64 v = *(u64 *)&input.buf[i-7];
            u64 idx = _pext_u64(v, pm);
            u32 table = primary_table[idx];
            if (unlikely(table)) {
                handle_table(input, table, i, out, result_idx);
            }
        }
        out.trim(result_idx);
    }
};

