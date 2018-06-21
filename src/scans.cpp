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
            cerr << "Could not allocate memory\n";
            exit(1);
        };
		is.read(aligned_buffer, length);
		is.close();
		return make_pair((u8 *)aligned_buffer, length);
	}
	else {
        cerr << "Couldn't open file: " << filename << "\n";
        exit(1);
    }
}

// replace with a JSON file containing my own signature format
// can build this up slowly
// use RapidJSON to slurp in this file
// starting scans are mostly acceleration-only so they could be passed in pretty easily
// on the command line

bool get_signatures(string filename, vector<Literal> & lits) {
	// reads 'filename', gets lits into vector
	// format is id, string/regex, flags, type
	// separator is : after id then / everywhere else
	ifstream is(filename);
	string s;
	while (std::getline(is, s)) {
		auto id_end = s.find_first_of(":");
		auto type_start = s.find_last_of("/");
		auto flags_start = s.find_last_of("/", type_start - 1);
		u32 id = atoi(s.substr(0, id_end).c_str());
		string val = s.substr(id_end + 2, flags_start - id_end - 2);
		string flags = s.substr(flags_start + 1, type_start - flags_start - 1);
		string type = s.substr(type_start + 1);
		assert(type == "lit");
		bool caseless = flags == "i"; // only flag we support
		// TODO: if caseless, need to squish 'val' and make it all upper case to match
		// our check semantics
		lits.emplace_back(Literal{ id, val, caseless });
	}
	return true;
}


void run_benchmarks(WrapperBase & w, InputBlock corpus, int repeats,
                    bool dump_times) {
    auto res = w.benchmark(corpus, repeats);
    if (dump_times) {
        copy(res.begin(), res.end(), ostream_iterator<double>(cout, "\n"));
    } 
    auto best = min_element(res.begin(), res.end());
    cout << "Best run: " << setw(2) << *best << "s or " 
         << (((double)corpus.second/1000000000) / *best) << " Gbytes/second\n";
}

void log_matcher(WrapperBase & w, InputBlock corpus) {
    auto out = w.log(corpus);
    copy(out.begin(), out.end(), ostream_iterator<u32>(cout, "\n"));
}

bool verify_matchers(WrapperBase & w, WrapperBase & w_gold, InputBlock corpus) {
    auto out = w.log(corpus);
    auto out_gold = w_gold.log(corpus);

    if (out != out_gold) {
        cout << "Error: results don't match\n";

        cout << "Matcher produced " << out.size() << " matches\n";
        cout << "Reference produced " << out_gold.size() << " matches\n";
        cout << "Results: \n";
        copy(out.begin(), out.end(), ostream_iterator<double>(cout, "\n"));
        cout << "\n";

        cout << "Results2: \n";
        copy(out_gold.begin(), out_gold.end(), ostream_iterator<double>(cout, "\n"));
        cout << "\n";
        return false;
    } else {
        cout << "Result OK. Matcher produced " << out.size() << " matches\n";
    }
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
    string charset("Zz"); // by default, something dumb
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
             | Opt ( charset, "charset")
                    ["--charset"]
                    ("Character set to scan for")
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

    auto corpus = get_corpus(corpus_file);

    try {
        auto w = get_wrapper(scanner_name, charset);
        if (!w) {
            cerr << "No such scanner: " << scanner_name << "\n";
            exit(1);
        }

        switch (command) {
        case LOG:
            log_matcher(*w, corpus);
            break;
        case VERIFY: {
            auto wg = get_ground_truth_wrapper(scanner_name, charset);
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
