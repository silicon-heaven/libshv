#include <necrolog.h>

#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>

#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>

#include <numeric>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

using namespace shv::chainpack;
using namespace shv::core::utils;
using namespace std;
/*
#define nFooInfo() nCInfo("foo")
#define nBarDebug() nCDebug("bar")

inline NecroLog &operator<<(NecroLog log, const std::vector<std::string> &sl)
{
	std::string s = std::accumulate(sl.begin(), sl.end(), std::string(),
										  [](const std::string& a, const std::string& b) -> std::string {
											  return a + (a.length() > 0 ? "," : "") + b;
										  } );
	return log << s;
}
*/
static const char* const cp2cp_help =
R"( SHV log converter

USAGE:
-i "indent_string"
	indent Cpon (default is no-indent "")
--ic --input-chainpack
	input stream is ChainPack [default]
--ip --input-cpon
	input stream is Cpon
--ig --input-shvlog
	input stream is SHV log
--op --output-cpon
	write output in CPon
--oc --output-chainpack
	write output in ChainPack
--og --output-shvlog [default]
	write output in SHV log

)";

namespace {
int replace_str(std::string& str, const std::string& from, const std::string& to)
{
	int i = 0;
	for (i = 0; ; ++i) {
		size_t start_pos = str.find(from);
		if(start_pos == std::string::npos)
			break;
		str.replace(start_pos, from.length(), to);
	}
	return i;
}

void help(const std::string &app_name)
{
	std::cout << app_name << cp2cp_help;
	std::cout << NecroLog::cliHelp();
	exit(0);
}
}

int main(int argc, char *argv[])
{
	std::vector<std::string> args = NecroLog::setCLIOptions(argc, argv);

	if(std::find(args.begin(), args.end(), "--help") != args.end()) {
		help(argv[0]);
	}
	nDebug() << NecroLog::thresholdsLogInfo();

	enum class Type {ChainPack, Cpon, ShvLog};
	std::string o_indent;
	Type o_input = Type::ChainPack;
	Type o_output = Type::ShvLog;
	std::string file_name;

	for (size_t i = 1; i < args.size(); ++i) {
		const std::string &arg = args[i];
		if(arg == "-i" && i < args.size() - 1) {
			std::string s = argv[++i];
			replace_str(s, "\\t", "\t");
			replace_str(s, "\\r", "\r");
			replace_str(s, "\\n", "\n");
			o_indent = s;
		}
		else if(arg == "--ic" || arg == "--input-chainpack")
			o_input = Type::ChainPack;
		else if(arg == "--ip" || arg == "--input-cpon")
			o_input = Type::Cpon;
		else if(arg == "--ig" || arg == "--input-shvlog")
			o_input = Type::ShvLog;
		else if(arg == "--oc" || arg == "--output-chainpack")
			o_output = Type::ChainPack;
		else if(arg == "--op" || arg == "--output-cpon")
			o_output = Type::Cpon;
		else if(arg == "--og" || arg == "--output-shvlog")
			o_output = Type::ShvLog;
		else if(arg == "-h")
			help(argv[0]);
		else
			file_name = arg;
	}

	std::istream *pin = nullptr;
	std::ifstream in_file;
	if(file_name.empty()) {
		nDebug() << "reading stdin";
		pin = &std::cin;
	}
	else {
		nDebug() << "reading:" << file_name;
		in_file.open(file_name, std::ios::in | std::ios::binary);
		if(!in_file) {
			nError() << "Cannot open" << file_name << "for reading";
			exit(-1);
		}
		pin = &in_file;
	}

	try {
		RpcValue log;
		switch(o_input) {
		case Type::ChainPack: {
			ChainPackReader rd(*pin);
			log = rd.read();
			break;
		}
		case Type::Cpon: {
			CponReader rd(*pin);
			log = rd.read();
			break;
		}
		case Type::ShvLog: {
			ShvJournalFileReader rd(*pin);
			RpcList records;
			while(rd.next()) {
				auto e = rd.entry();
				records.push_back(e.toRpcValueList());
			}
			log = RpcValue(std::move(records));
			break;
		}
		}
		switch(o_output) {
		case Type::ChainPack: {
			ChainPackWriter wr(std::cout);
			wr.write(log);
			break;
		}
		case Type::Cpon: {
			shv::chainpack::CponWriterOptions opts;
			opts.setIndent(o_indent);
			CponWriter wr(std::cout, opts);
			wr.write(log);
			break;
		}
		case Type::ShvLog: {
			ShvJournalFileWriter wr(std::cout);
			for(const auto &e : log.asList()) {
				auto entry = ShvJournalEntry::fromRpcValueList(e.asList());
				wr.append(entry);
			}
			break;
		}
		}
	}
	catch (std::exception &e) {
		nError() << e.what();
		exit(-1);
	}

	return 0;
}
