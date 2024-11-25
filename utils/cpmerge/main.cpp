#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/utils.h>

#include <iostream>
#include <fstream>
#include <cstring>

namespace {
const auto cpmerge_help =
"\n"
"Cpmerge merge stdin with config.file. Merged file put to stdout\n"
"\n"
"USAGE: cpmerge config.file\n";
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "No argument. Use config.file as first argument.\n";
		std::cout << cpmerge_help;
		return 1;
	}

	for(int i=1; i<argc; i++) {
		const char *arg = argv[i];
		if(strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
			std::cout << cpmerge_help;
			return 0;
		}
	}

	std::ifstream fis;
	fis.open(argv[1]);
	if (!fis.good()) {
		std::cerr << "Cannot open config file for reading.\n";
		return 1;
	}
	std::string err;
	shv::chainpack::RpcValue template_config = shv::chainpack::CponReader(fis).read(&err);
	fis.close();

	if (!err.empty()){
		std::cerr << err << '\n';
		return 2;
	}

	shv::chainpack::RpcValue overlay_config = shv::chainpack::CponReader(std::cin).read(&err);

	if (!err.empty()){
		std::cerr << err << '\n';
		return 3;
	}

	shv::chainpack::RpcValue rv = shv::chainpack::Utils::mergeMaps(template_config.asMap(), overlay_config.asMap());

	shv::chainpack::CponWriterOptions opts;
	opts.setIndent("\t");
	shv::chainpack::CponWriter wr(std::cout, opts);
	wr << rv;
	return 0;
}
