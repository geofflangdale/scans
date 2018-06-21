#pragma once

#include <memory>
#include <set>
#include "scans.h"

// this file just contains public interfaces to get wrappers defined in other files
// the idea is that we don't want to have to include all the scans code in scans.cpp; by default, it
// should only need to be included in <scan_name>.cpp
// this isn't that convenient but it only needs to be changed when we add a new bit of functionality

std::unique_ptr<WrapperBase> get_wrapper(std::string name, std::string workload);

std::unique_ptr<WrapperBase> get_ground_truth_wrapper(std::string name, std::string workload);

