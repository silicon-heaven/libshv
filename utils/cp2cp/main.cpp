#include <necrolog.h>

#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>

#include <numeric>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

namespace cp = shv::chainpack;
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
R"( ChainPack to Cpon converter

USAGE:
-i "indent_string"
	indent Cpon (default is no-indent "")
-t
	human readable metatypes in Cpon output
--ic --input-chainpack
	input stream is ChainPack [default]
--ip --input-cpon
	input stream is Cpon
--op --output-cpon
	write output in CPon [default]
--oc --output-chainpack
	write output in ChainPack

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

	enum class Format {ChainPack, Cpon};
	std::string o_indent;
	bool o_translate_meta_ids = false;
	Format o_input = Format::ChainPack;
	Format o_output = Format::Cpon;
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
		else if(arg == "-t")
			o_translate_meta_ids = true;
		else if(arg == "--ic" || arg == "--input-chainpack")
			o_input = Format::ChainPack;
		else if(arg == "--ip" || arg == "--input-cpon")
			o_input = Format::Cpon;
		else if(arg == "--oc" || arg == "--output-chainpack")
			o_output = Format::ChainPack;
		else if(arg == "--op" || arg == "--output-cpon")
			o_output = Format::Cpon;
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


	cp::AbstractStreamReader *prd;
	cp::AbstractStreamWriter *pwr;

	if(o_input == Format::Cpon) {
		nMessage() << "from Cpon";
		prd = new cp::CponReader(*pin);
	}
	else {
		nMessage() << "from ChainPack";
		prd = new cp::ChainPackReader(*pin);
	}

	if(o_output == Format::ChainPack) {
		nMessage() << "to ChainPack";
		pwr = new cp::ChainPackWriter(std::cout);
	}
	else {
		nMessage() << "to Cpon";
		shv::chainpack::CponWriterOptions opts;
		opts.setIndent(o_indent);
		opts.setTranslateIds(o_translate_meta_ids);
		auto *wr = new cp::CponWriter(std::cout, opts);
		pwr = wr;
	}

	try {
		nMessage() << "read";
		shv::chainpack::RpcValue val = prd->read();
		nMessage() << "write";
		pwr->write(val);
	}
	catch (std::exception &e) {
		nError() << e.what();
		exit(-1);
	}

	delete prd;
	delete pwr;

	return 0;
}
