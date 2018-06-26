#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <iterator>
#include <algorithm>
#include <stdexcept>

#include <clara.hpp>

#include "common_defs.h"
#include "compare.h"
#include "literal.h"
#include "scans.h"

#include "public.h"

using namespace std;

InputBlock get_corpus(string filename) {
	ifstream is(filename, ios::binary);
	if (is) {
		// get length of file:
		is.seekg(0, is.end);
		size_t length = (size_t)is.tellg();
		is.seekg(0, is.beg);

        char * aligned_buffer;
        if (posix_memalign( (void **)&aligned_buffer, 64, ROUNDUP_N(length, 64))) {
            cerr << "Could not allocate memory for corpus\n";
            exit(1);
        };
		is.read(aligned_buffer, length);
		is.close();
		return InputBlock((u8 *)aligned_buffer, length, 0, length);
	}
	else {
        cerr << "Couldn't open corpus file: " << filename << "\n";
        exit(1);
    }
}

void run_benchmarks(WrapperBase & w, InputBlock corpus, int repeats,
                    bool dump_times) {
    auto res = w.benchmark(corpus, repeats);
    if (dump_times) {
        copy(res.begin(), res.end(), ostream_iterator<double>(cout, "\n"));
    } 
    auto best = min_element(res.begin(), res.end());
    cout << "Best run: " << setw(2) << *best << "s or " 
         << (((double)corpus.bytes_to_scan()/1000000000) / *best) << " Gbytes/second\n";
}

void log_matcher(WrapperBase & w, InputBlock corpus) {
    auto out = w.log(corpus);
    out->dump_results(cout);
    delete out;
}

bool verify_matchers(WrapperBase & w, WrapperBase & w_gold, InputBlock corpus) {
    auto out = w.log(corpus);
    auto out_gold = w_gold.log(corpus);

    if (*out != *out_gold) {
        cout << "Error: results don't match\n";

        cout << "Matcher produced " << out->get_size() << " matches\n";
        cout << "Reference produced " << out_gold->get_size() << " matches\n";
        cout << "Matcher results: \n";
        out->dump_results(cout);
        cout << "\n";

        cout << "Reference results: \n";
        out_gold->dump_results(cout);
        cout << "\n";
        return false;
    } else {
        cout << "Result OK. Matcher produced " << out->get_size() << " matches\n";
    }
    delete out;
    delete out_gold;
    return true;
}

int main(int argc, char * argv[]) {
    using namespace clara;
#if (defined(DEBUG))
    int repeats = 1;
#else
    int repeats = 100;
#endif
    bool show_help = false;
    bool dump_times = false;
    string scanner_name = "truffle";
    typedef enum { BENCHMARK, VERIFY, LOG } CommandType;
    CommandType command = BENCHMARK;
    string corpus_file("in.txt");
    string workload; // by default, something dumb
    string workload_file; // by default, something dumb
    auto cli = Help (show_help)
             | Opt( repeats, "repeats" )
                    ["-r"]["--repeats"]
                    ("Number of repeated scans")
             | Opt( dump_times )
                    ["--dumptimes"]
                    ("Dump all times to stdout")
             | Opt (scanner_name, "scanner_name")
                    ["-s"]["--scanner"] 
                    ("Which scanner to use")
             | Opt ( workload, "workload")
                    ["-w"]["--workload"]
                    ("String set or character set to scan for")
             | Opt ( workload_file, "workload_file")
                    ["--workload-file"]
                    ("A file containing the string set or character set to scan for")
             | Opt( corpus_file, "corpus" )
                    ["-c"]["--corpus"]
                    ("Name of corpus file")
             | Arg( [&]( string cmd) {
                        map<string, CommandType> cmd_map = {
                            {"benchmark", BENCHMARK},
                            {"verify", VERIFY},
                            {"log", LOG}
                        };
                        if (cmd_map.find(cmd) == cmd_map.end()) {
                            return ParserResult::runtimeError("Unknown command '" + cmd + "'");
                        } else {
                            command = cmd_map[cmd];
                            return ParserResult::ok(ParseResultType::Matched);
                        }
                    }, "benchmark|verify|log")
                    ("Which command to run");

    auto result = cli.parse( Args( argc, argv ) );
    if( !result ) {
        cerr << "Error in command line: " << result.errorMessage() << "\n";
        exit(1);
    }

    if (!repeats) {
        cerr << "Can't have 0 repeats\n";
        exit(1);
    }
    
    if (show_help) {
        cerr << cli << "\n";
        exit(1);
    }
    if (workload.empty() && workload_file.empty()) {
        cerr << "Must specify either a command-line workload or a workload file\n";
        exit(1);
    }

    if (!workload.empty() && !workload_file.empty()) {
        cerr << "Cannot specify both a command-line workload or a workload file\n";
        exit(1);
    }

    if (!workload_file.empty()) {
        std::ifstream t(workload_file);
        if (!t) {
            cerr << "Couldn't open workload file: " << workload_file << "\n";
            exit(1);
        }
        std::stringstream buf;
        buf << t.rdbuf(); 
        workload = buf.str();
    }

    auto corpus = get_corpus(corpus_file);

    try {
        auto w = get_wrapper(scanner_name, workload);
        if (!w) {
            cerr << "No such scanner: " << scanner_name << "\n";
            exit(1);
        }

        switch (command) {
        case LOG:
            log_matcher(*w, corpus);
            break;
        case VERIFY: {
            auto wg = get_ground_truth_wrapper(scanner_name, workload);
            verify_matchers(*w, *wg, corpus);
            break;
        }
        case BENCHMARK:
            run_benchmarks(*w, corpus, repeats, dump_times);
            break;
        }
    }
    catch (const exception& e) {
        cerr << "Could not build scanner: " << e.what() << "\n";
    }

}
