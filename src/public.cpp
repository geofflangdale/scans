#include "public.h"

#include <memory>
#include <map>
#include <set>
#include <tuple>

#include <iostream>
using namespace std;

std::unique_ptr<WrapperBase> get_wrapper_truffle(const std::set<u8> & in);
std::unique_ptr<WrapperBase> get_wrapper_shufti(const std::set<u8> & in);
std::unique_ptr<WrapperBase> get_wrapper_vermicelli(const std::set<u8> & in);
std::unique_ptr<WrapperBase> get_wrapper_charsetgold(const std::set<u8> & in);

std::unique_ptr<WrapperBase> get_wrapper_dverm(const DoubleCharsetWorkload & in);

// add a proper parser later
set<u8> workload_to_charset(string s) {
    return set<u8>(s.begin(), s.end());
}

DoubleCharsetWorkload workload_to_double_char_set(string s) {
    // format: set/set/distance
    // so: a/b/3 is equivalent to a..b
    // distance = 0 is illegal
    auto set1_end = s.find_first_of("/");
    if (set1_end == string::npos) {
        throw logic_error("Couldn't parse double char set argument");
    }
    auto set2_end = s.find_first_of("/", set1_end+1);
    if (set2_end == string::npos) {
        throw logic_error("Couldn't parse double char set argument");
    }
        
    set<u8> s1(s.begin(), s.begin() + set1_end);
    set<u8> s2(s.begin() + set1_end + 1, s.begin() + set2_end);
    u32 dist = atoi(s.substr(set2_end+1, string::npos).c_str());
    if (!dist) {
        throw logic_error("Distance is zero or invalid");
    }
    return make_tuple(s1, s2, dist);
}

unique_ptr<WrapperBase> get_wrapper(string name, string workload) {
    // just copy our workload string as a char set for now
    set<u8> in(workload.begin(), workload.end());
    if (name == "truffle") {
        return get_wrapper_truffle(workload_to_charset(workload));
    } else if (name == "charsetgold") {
        return get_wrapper_charsetgold(workload_to_charset(workload));
    } else if (name == "shufti") {
        return get_wrapper_shufti(workload_to_charset(workload));
    } else if (name == "vermicelli") {
        return get_wrapper_vermicelli(workload_to_charset(workload));
    } else if (name == "dverm") {
        return get_wrapper_dverm(workload_to_double_char_set(workload));
    }
    throw logic_error("No such matcher exists: " + name);
}

unique_ptr<WrapperBase> get_ground_truth_wrapper(string name, string workload) {
    map<string, string> m = {
        { "truffle", "charsetgold" },
        { "shufti", "charsetgold" },
        { "vermicelli", "charsetgold" },
        { "dverm", "doublesetgold" }
    };
    if (m.find(name) == m.end()) {
        throw logic_error("No ground truth matcher for " + name);
    }
    return get_wrapper(m[name], workload);
}
