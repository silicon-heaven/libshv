#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/shvtypeinfo.h>
#include <shv/core/log.h>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <filesystem>

namespace fs = std::filesystem;
using namespace shv::chainpack;
using namespace shv::core::utils;
using namespace std;

namespace {
RpcValue read_cpon_file(const string &fn)
{
	shvInfo() << "reading file: " << fn;
	ifstream in(fn, std::ios::binary | std::ios::in);
	if(in) {
		CponReader rd(in);
		return rd.read();
	}

	throw runtime_error("cannot open file: " + fn + " for reading");
}

void write_cpon_file(const string &fn, const RpcValue &log)
{
	ofstream out(fn, std::ios::binary | std::ios::out);
	if(out) {
		shvInfo() << "writing file: " << fn;
		CponWriterOptions opts;
		opts.setIndent("\t");
		CponWriter wr(out, opts);
		wr << log;
	}
	else {
		throw runtime_error("cannot open file: " + fn + " for writing");
	}
}

const std::string FILES_DIR = DEF_FILES_DIR;
const std::string TEST_DIR = "test_ShvTypeInfo";
}

int main(int argc, char** argv)
{
	auto tmp_path = fs::temp_directory_path().string();
	fs::current_path(tmp_path);
	fs::remove_all(TEST_DIR);
	fs::create_directory(TEST_DIR);

	return doctest::Context(argc, argv).run();
}

DOCTEST_TEST_CASE("ShvTypeInfo")
{
	auto tmp_path = fs::temp_directory_path().string();
	auto out_path = tmp_path + '/' + TEST_DIR;

	DOCTEST_SUBCASE("nodesTree to typeInfo conversion")
	{
		{
			auto rv = read_cpon_file(FILES_DIR + "/nodesTree.cpon");
			auto type_info = ShvTypeInfo::fromRpcValue(rv);
			auto typeinfo_file_path = out_path + "/typeInfo.cpon"s;
			write_cpon_file(typeinfo_file_path, type_info.toRpcValue());
			DOCTEST_SUBCASE("Path info")
			{
				auto type_info2 = ShvTypeInfo::fromRpcValue(rv);
				REQUIRE(type_info.toRpcValue() == type_info2.toRpcValue());
			}
			{
				auto pi = type_info.pathInfo("devices/tc/TC01");
				REQUIRE(pi.deviceType == "TC_G3");
				REQUIRE(pi.devicePath == "devices/tc/TC01");
				REQUIRE(!pi.propertyDescription.isValid());
				REQUIRE(pi.propertyDescription.name().empty());
				REQUIRE(pi.fieldPath.empty());
			}
			{
				auto pi = type_info.pathInfo("devices/tc/TC01/status");
				// auto s = pi.propertyDescription.toRpcValue().toCpon();
				REQUIRE(pi.propertyDescription.isValid());
				REQUIRE(pi.deviceType == "TC_G3");
				REQUIRE(pi.devicePath == "devices/tc/TC01");
				REQUIRE(pi.propertyDescription.typeName() == "StatusTC");
				REQUIRE(pi.propertyDescription.name() == "status");
				REQUIRE(pi.fieldPath.empty());
			}
			{
				auto pi = type_info.pathInfo("devices/tc/TC01/status/occupied");
				REQUIRE(pi.deviceType == "TC_G3");
				REQUIRE(pi.devicePath == "devices/tc/TC01");
				REQUIRE(pi.propertyDescription.typeName() == "StatusTC");
				REQUIRE(pi.propertyDescription.name() == "status");
				REQUIRE(pi.fieldPath == "occupied");
			}
			{
				// nested device
				auto pi = type_info.pathInfo("devices/zone/langevelden/route/AB/status");
				REQUIRE(pi.deviceType == "Route_G3");
				REQUIRE(pi.devicePath == "devices/zone/langevelden/route/AB");
				REQUIRE(pi.propertyDescription.typeName() == "StatusRoute");
				REQUIRE(pi.propertyDescription.name() == "status");
				REQUIRE(pi.fieldPath.empty());
			}
			{
				// nested device
				auto pi = type_info.pathInfo("devices/zone/langevelden/route/AB/status/routeState");
				REQUIRE(pi.deviceType == "Route_G3");
				REQUIRE(pi.devicePath == "devices/zone/langevelden/route/AB");
				REQUIRE(pi.propertyDescription.typeName() == "StatusRoute");
				REQUIRE(pi.propertyDescription.name() == "status");
				REQUIRE(pi.fieldPath == "routeState");

				auto td = type_info.findTypeDescription(pi.propertyDescription.typeName());
				REQUIRE(td.type() == ShvTypeDescr::Type::BitField);
				auto fd = td.field(pi.fieldPath);
				REQUIRE(fd.typeName() == "EnumRouteState");

				auto td2 = type_info.findTypeDescription(fd.typeName());
				REQUIRE(td2.type() == ShvTypeDescr::Type::Enum);
				auto fd2 = td2.field("Ready");
				REQUIRE(fd2.bitRange() == 3);
				REQUIRE(fd2.label() == "Ready");
			}
			{
				// extra tags
				auto et = type_info.extraTagsForPath("devices/tc/TC04");
				REQUIRE(et.asMap().value("brclab").asMap().value("url").asString() == "brclab://192.168.1.10:4000/4");
			}
			{
				// extra tags
				auto et = type_info.extraTagsForPath("devices/zone/langevelden/method/setNormal");
				REQUIRE(et.asMap().value("safetyManager").asString() == "systemSafety");
			}
			{
				// forEachNode
				type_info.forEachProperty([](const std::string &shv_path, const ShvPropertyDescr &node_descr) {
					CAPTURE(shv_path  + " --> " + node_descr.typeName());
					if(!shv_path.empty())
						REQUIRE(shv_path[0] != std::toupper(shv_path[0]));
				});
			}
		}
		DOCTEST_SUBCASE("Node description deviations")
		{
			auto typeinfo_file_path = out_path + "/typeInfo2.cpon"s;
			ShvTypeInfo type_info;
			DOCTEST_SUBCASE("Original typeinfo")
			{
				auto rv = read_cpon_file(FILES_DIR + "/nodesTree.cpon");
				type_info = ShvTypeInfo::fromRpcValue(rv);
				write_cpon_file(typeinfo_file_path, type_info.toRpcValue());
				{
					auto td = type_info.findTypeDescription("StatusTC");
					REQUIRE(td.type() == ShvTypeDescr::Type::BitField);
					REQUIRE(td.typeName() == "BitField");
				}
			}
			DOCTEST_SUBCASE("Reloaded typeinfo")
			{
				auto rv2 = read_cpon_file(typeinfo_file_path);
				auto type_info2 = ShvTypeInfo::fromRpcValue(rv2);
				{
					auto td = type_info2.findTypeDescription("StatusTC");
					REQUIRE(td.type() == ShvTypeDescr::Type::BitField);
					REQUIRE(td.typeName() == "BitField");
				}
				{
					auto pi = type_info2.pathInfo("devices/signal/SA04/symbol/RED_LEFT/status");
					REQUIRE(pi.deviceType == "SignalSymbol_G3");
					REQUIRE(pi.devicePath == "devices/signal/SA04/symbol/RED_LEFT");
					REQUIRE(pi.propertyDescription.typeName() == "StatusSignalSymbol");
					REQUIRE(pi.propertyDescription.name() == "status");
					REQUIRE(pi.fieldPath.empty());
				}
				{
					auto pi = type_info2.pathInfo("devices/signal/SA04/symbol/WHITE/status");
					REQUIRE(pi.deviceType == "SignalSymbol_G3");
					REQUIRE(pi.devicePath == "devices/signal/SA04/symbol/WHITE");
					REQUIRE(pi.propertyDescription.typeName() == "StatusSignalSymbolWhite");
					REQUIRE(pi.propertyDescription.name() == "status");
					REQUIRE(pi.fieldPath.empty());
				}
				{
					auto pi = type_info2.pathInfo("devices/signal/SG01/symbol/P80Y");
					REQUIRE(pi.deviceType == "SignalSymbol_G3");
					REQUIRE(pi.devicePath == "devices/signal/SG01/symbol/P80Y");
					REQUIRE(pi.propertyDescription.typeName().empty());
					REQUIRE(pi.propertyDescription.name().empty());
					REQUIRE(pi.fieldPath.empty());
				}
				REQUIRE(type_info2.propertyDescriptionForPath("devices/signal/SG01/symbol/P80Y/status").typeName() == "StatusSignalSymbolP80Y");
			}
		}
	}

	DOCTEST_SUBCASE("Invalid result for propertyDescriptionForPath(...) function")
	{
		auto rv = read_cpon_file(FILES_DIR + "/hel002_nodesTree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(out_path + "/hel002_typeInfo.cpon", type_info.toRpcValue());

		// Node descriptions
		{
			string field_name;
			auto nd = type_info.propertyDescriptionForPath("heating/group/ESHS_Z1/status/errorAutomaticControl", &field_name);
			INFO("propertyDescriptionForPath: ", nd.toRpcValue().toCpon());
			//CAPTURE(nd.toRpcValue().toCpon());
			REQUIRE(nd.isValid() == true);
			REQUIRE(nd.typeName() == "StatusHeatingGroupNew");
			REQUIRE(field_name == "errorAutomaticControl");
		}
		// System paths
		{
			REQUIRE(type_info.systemPathsRoots().empty() == false);
			REQUIRE(type_info.findSystemPath("a/b/c") == "system/sig");
			REQUIRE(type_info.findSystemPath("elbox/VL1-1") == "system/eshs");
			REQUIRE(type_info.findSystemPath("elbox/VL1-1/status") == "system/eshs");
		}
		{
			// extra tags should not be valid
			for(const auto &[key, val] : type_info.extraTags()) {
				REQUIRE(val.isValid());
			}
		}
	}

	DOCTEST_SUBCASE("Extra property in conflicting device type definition")
	{
		auto rv = read_cpon_file(FILES_DIR + "/hel002_z2_nodesTree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(out_path + "/hel002_z2_typeInfo.cpon", type_info.toRpcValue());

		{
			auto pi = type_info.pathInfo("elbox/VL2-2/temperature");
			INFO("propertyDescriptionForPath: ", pi.propertyDescription.toRpcValue().toCpon());
			REQUIRE(pi.deviceType == "ElboxHeating");
			REQUIRE(pi.propertyDescription.isValid() == true);
		}
		{
			auto pi = type_info.pathInfo("elbox/VL2-3/temperature");
			INFO("propertyDescriptionForPath: ", pi.propertyDescription.toRpcValue().toCpon());
			REQUIRE(pi.deviceType == "ElboxHeating");
			REQUIRE(pi.propertyDescription.isValid() == false);
		}
	}

	DOCTEST_SUBCASE("Missing property in conflicting device type definition")
	{
		auto rv = read_cpon_file(FILES_DIR + "/hel002_z5_nodesTree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(out_path + "/hel002_z5_typeInfo.cpon", type_info.toRpcValue());

		{
			auto pi = type_info.pathInfo("elbox/VL5-2/temperature");
			INFO("propertyDescriptionForPath: ", pi.propertyDescription.toRpcValue().toCpon());
			REQUIRE(pi.deviceType == "ElboxHeating");
			REQUIRE(pi.propertyDescription.isValid() == true);
		}
		{
			auto pi = type_info.pathInfo("elbox/VL5-3/temperature");
			INFO("propertyDescriptionForPath: ", pi.propertyDescription.toRpcValue().toCpon());
			CAPTURE(pi.propertyDescription .toRpcValue().toCpon());
			REQUIRE(pi.deviceType == "ElboxHeating");
			REQUIRE(!pi.propertyDescription.isValid());
		}
	}

	DOCTEST_SUBCASE("Crash loading some nodes-tree files")
	{
		auto rv = read_cpon_file(FILES_DIR + "/ols007_nodesTree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(out_path + "/ols007_typeInfo.cpon", type_info.toRpcValue());

	}

	DOCTEST_SUBCASE("Cannot resolve TC/status/occupied, if createFromTypeName tag is missing")
	{
		auto rv = read_cpon_file(FILES_DIR + "/barrandov-nodestree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(out_path + "/barrandov_typeInfo.cpon", type_info.toRpcValue());
		string fld_name;
		auto pd = type_info.propertyDescriptionForPath("tc/TC07/status/occupied", &fld_name);
		REQUIRE(pd.isValid());
		REQUIRE(fld_name == "occupied");
	}

	DOCTEST_SUBCASE("Blacklists")
	{
		auto rv = read_cpon_file(FILES_DIR + "/ghe022_nodesTree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(out_path + "/ghe022_typeInfo.cpon", type_info.toRpcValue());

		{
			REQUIRE(type_info.isPathBlacklisted("devices/signal/SA03/symbol/WHITE/config/currentLimitError") == true);
			REQUIRE(type_info.isPathBlacklisted("foo/bar") == false);

			auto pi = type_info.pathInfo("devices/tc/TC01/status/claimed");
			REQUIRE(pi.devicePath == "devices/tc/TC01");
			REQUIRE(pi.propertyDescription.name() == "status");
			REQUIRE(pi.fieldPath == "claimed");

			REQUIRE(type_info.isPathBlacklisted("devices/tc/TC01/status") == false);
			REQUIRE(type_info.isPathBlacklisted("devices/tc/TC01/status/") == false);
			REQUIRE(type_info.isPathBlacklisted("devices/tc/TC01/status/claimed") == true);
			REQUIRE(type_info.isPathBlacklisted("devices/tc/TC01/status/claimed/") == true);
			REQUIRE(type_info.isPathBlacklisted("devices/tc/TC01/status/claimed/foo") == true);
			REQUIRE(type_info.isPathBlacklisted("devices/tc/TC01/status/claimed/foo/") == true);
		}
	}
}
