#pragma once

#include <memory>
#include <set>
#include "scans.h"
// this file just contains public interfaces to get benchmarkers/loggers defined in other files
// the idea is that we don't want to have to include all the scans code in scans.cpp; by default, it
// should only need to be included in <scan_name>.cpp
// this isn't that convenient but it only needs to be changed when we add a new bit of functionality

// TODO: what we really should do is boot this stuff into a .cpp file and load up all of these 
// handles to be accessed by string-based names; it should be possible to extract a description
// as well as get either a logger or a benchmarker

class Truffle;
std::unique_ptr<LoggerBase> get_logger_truffle(const std::set<u8> & in);
std::unique_ptr<BenchmarkerBase> get_benchmarker_truffle(const std::set<u8> & in);

class CharsetGold;
std::unique_ptr<LoggerBase> get_logger_charsetgold(const std::set<u8> & in);
std::unique_ptr<BenchmarkerBase> get_benchmarker_charsetgold(const std::set<u8> & in);

class Shufti;
std::unique_ptr<LoggerBase> get_logger_shufti(const std::set<u8> & in);
std::unique_ptr<BenchmarkerBase> get_benchmarker_shufti(const std::set<u8> & in);
