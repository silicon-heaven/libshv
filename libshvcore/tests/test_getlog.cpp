#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <shv/core/utils/getlog.h>
#include <shv/core/utils/shvgetlogparams.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/log.h>

#include <filesystem>

namespace cp = shv::chainpack;
using cp::RpcValue;
using namespace std::string_literals;

namespace {
// https://stackoverflow.com/a/56766138
template <typename T>
constexpr auto type_name()
{
    std::string_view name, prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "auto type_name() [T = ";
    suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto type_name() [with T = ";
    suffix = "]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "auto __cdecl type_name<";
    suffix = ">(void)";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}
}

namespace std {
// NOLINTBEGIN(misc-use-internal-linkage)
    template <typename Type>
    doctest::String toString(const std::vector<Type>& values) {
        std::ostringstream res;
        res << "std::vector<" << type_name<Type>() << ">{\n";
        for (const auto& value : values) {
            if constexpr (std::is_same<Type, shv::core::utils::ShvJournalEntry>()) {
                res << "    " << value.toRpcValue().toCpon() << ",\n";
            } else {
                res << "    " << value << ",\n";
            }
        }
        res << "}";
        return res.str().c_str();
    }
}
// NOLINTEND(misc-use-internal-linkage)

namespace {
const auto journal_dir = std::filesystem::path{TEST_FILES_DIR} / "getlog";

std::function<shv::core::utils::ShvJournalFileReader()> create_reader(std::vector<shv::core::utils::ShvJournalEntry> entries)
{
	if (entries.empty()) {
		return {};
	}
	shv::core::utils::ShvJournalFileWriter writer(journal_dir.string(), entries.begin()->epochMsec, entries.back().epochMsec);

	for (const auto& entry : entries) {
		writer.append(entry);
	}

	return [file_name = writer.fileName()] {
		return shv::core::utils::ShvJournalFileReader{file_name};
	};
}

auto make_entry(const std::string& timestamp, const std::string& path, const RpcValue& value, bool snapshot)
{
	auto res = shv::core::utils::ShvJournalEntry(
		path,
		value,
		shv::core::utils::ShvJournalEntry::DOMAIN_VAL_CHANGE,
		shv::core::utils::ShvJournalEntry::NO_SHORT_TIME,
		shv::core::utils::ShvJournalEntry::NO_VALUE_FLAGS,
		RpcValue::DateTime::fromIsoString(timestamp).msecsSinceEpoch());

	res.setSnapshotValue(snapshot);
	return res;
}

auto as_vector(shv::core::utils::ShvLogRpcValueReader& reader)
{
	std::vector<shv::core::utils::ShvJournalEntry> res;
	while (reader.next()) {
		res.push_back(reader.entry());
	}
	return res;
}
}

DOCTEST_TEST_CASE("getLog")
{
	NecroLog::setTopicsLogThresholds("GetLog:D,ShvJournal:D");
    shv::core::utils::ShvGetLogParams get_log_params;
	std::filesystem::remove_all(journal_dir);
	std::filesystem::create_directories(journal_dir);

	DOCTEST_SUBCASE("filtering")
	{
		std::vector<std::function<shv::core::utils::ShvJournalFileReader()>> readers {
			create_reader({
				make_entry("2022-07-07T18:06:15.557Z", "APP_START", true, false),
				make_entry("2022-07-07T18:06:17.784Z", "zone1/system/sig/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.784Z", "zone1/zone/Zone1/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.869Z", "zone1/pme/TSH1-1/switchRightCounterPermanent", 0U, false),
			}),
			create_reader({
				make_entry("2022-07-07T18:06:17.872Z", "zone1/system/sig/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.874Z", "zone1/zone/Zone1/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.880Z", "zone1/pme/TSH1-1/switchRightCounterPermanent", 0U, false),
			})
		};

		DOCTEST_SUBCASE("since/until")
		{
			std::vector<std::string> expected_timestamps;

			DOCTEST_SUBCASE("default params")
			{
				expected_timestamps = {
					"2022-07-07T18:06:15.557Z",
					"2022-07-07T18:06:17.784Z",
					"2022-07-07T18:06:17.784Z",
					"2022-07-07T18:06:17.869Z",
					"2022-07-07T18:06:17.872Z",
					"2022-07-07T18:06:17.874Z",
					"2022-07-07T18:06:17.880Z",
				};
			}

			DOCTEST_SUBCASE("since")
			{
				expected_timestamps = {
					"2022-07-07T18:06:17.874Z",
					"2022-07-07T18:06:17.880Z",
				};
				get_log_params.since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.872Z");
			}

			DOCTEST_SUBCASE("until")
			{
				DOCTEST_SUBCASE("until in between entries")
				{
					expected_timestamps = {
						"2022-07-07T18:06:15.557Z",
						"2022-07-07T18:06:17.784Z",
						"2022-07-07T18:06:17.784Z",
						"2022-07-07T18:06:17.869Z",
					};
					get_log_params.until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.872Z");
				}

				DOCTEST_SUBCASE("until after last entry")
				{
					expected_timestamps = {
						"2022-07-07T18:06:15.557Z",
						"2022-07-07T18:06:17.784Z",
						"2022-07-07T18:06:17.784Z",
						"2022-07-07T18:06:17.869Z",
						"2022-07-07T18:06:17.872Z",
						"2022-07-07T18:06:17.874Z",
						"2022-07-07T18:06:17.880Z",
					};
					get_log_params.until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.900");
				}

				DOCTEST_SUBCASE("until ON the last entry")
				{
					expected_timestamps = {
						"2022-07-07T18:06:15.557Z",
						"2022-07-07T18:06:17.784Z",
						"2022-07-07T18:06:17.784Z",
						"2022-07-07T18:06:17.869Z",
						"2022-07-07T18:06:17.872Z",
						"2022-07-07T18:06:17.874Z",
					};
					get_log_params.until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.880");
				}

			}
			std::vector<std::string> actual_timestamps;
			shv::core::utils::ShvLogRpcValueReader entries(getLog(readers, get_log_params, RpcValue::DateTime::fromIsoString("2024-07-07T18:06:20.850")));
			for (const auto& entry : as_vector(entries)) {
				actual_timestamps.push_back(RpcValue::DateTime::fromMSecsSinceEpoch(entry.epochMsec).toIsoString());
			}

			REQUIRE(actual_timestamps == expected_timestamps);
		}

		DOCTEST_SUBCASE("pattern")
		{
			std::vector<std::string> expected_paths;

			DOCTEST_SUBCASE("default params")
			{
				expected_paths = {
					"APP_START",
					"zone1/system/sig/plcDisconnected",
					"zone1/zone/Zone1/plcDisconnected",
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
					"zone1/system/sig/plcDisconnected",
					"zone1/zone/Zone1/plcDisconnected",
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
				};
			}

			DOCTEST_SUBCASE("exact path")
			{
				get_log_params.pathPattern = "zone1/pme/TSH1-1/switchRightCounterPermanent";
				expected_paths = {
					// There are two entries with this path
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
				};
			}

			DOCTEST_SUBCASE("wildcard")
			{
				get_log_params.pathPattern = "zone1/**";
				expected_paths = {
					"zone1/system/sig/plcDisconnected",
					"zone1/zone/Zone1/plcDisconnected",
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
					"zone1/system/sig/plcDisconnected",
					"zone1/zone/Zone1/plcDisconnected",
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
				};
			}

			std::vector<std::string> actual_paths;
			shv::core::utils::ShvLogRpcValueReader entries(getLog(readers, get_log_params, RpcValue::DateTime::fromIsoString("2024-07-07T18:06:20.850")));
			for (const auto& entry : as_vector(entries)) {
				actual_paths.push_back(entry.path);
			}

			REQUIRE(actual_paths == expected_paths);
		}

		DOCTEST_SUBCASE("record count limit")
		{
			int expected_count;
			bool expected_record_count_limit_hit;

			DOCTEST_SUBCASE("default")
			{
				expected_count = 7;
				expected_record_count_limit_hit = false;
			}

			DOCTEST_SUBCASE("1000")
			{
				get_log_params.recordCountLimit = 1000;
				expected_count = 7;
				expected_record_count_limit_hit = false;
			}

			DOCTEST_SUBCASE("7")
			{
				get_log_params.recordCountLimit = 7;
				expected_count = 7;
				expected_record_count_limit_hit = false;
			}

			DOCTEST_SUBCASE("3")
			{
				get_log_params.recordCountLimit = 3;
				expected_count = 3;
				expected_record_count_limit_hit = true;
			}

			DOCTEST_SUBCASE("2")
			{
				get_log_params.recordCountLimit = 2;
				expected_count = 3;
				expected_record_count_limit_hit = true;
			}

			shv::core::utils::ShvLogRpcValueReader entries(getLog(readers, get_log_params, RpcValue::DateTime::fromIsoString("2024-07-07T18:06:20.850")));

			REQUIRE(entries.logHeader().recordCountLimitHit() == expected_record_count_limit_hit);
			REQUIRE(as_vector(entries).size() == expected_count);
		}
	}

	DOCTEST_SUBCASE("withPathsDict")
	{
		std::vector<std::function<shv::core::utils::ShvJournalFileReader()>> readers {
			create_reader({
				make_entry("2022-07-07T18:06:15.557Z", "APP_START", true, false),
				make_entry("2022-07-07T18:06:17.784Z", "zone1/system/sig/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.784Z", "zone1/zone/Zone1/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.869Z", "zone1/pme/TSH1-1/switchRightCounterPermanent", 0U, false),
			}),
			create_reader({
				make_entry("2022-07-07T18:06:17.872Z", "zone1/system/sig/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.874Z", "zone1/zone/Zone1/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.880Z", "zone1/pme/TSH1-1/switchRightCounterPermanent", 0U, false),
			})
		};

		DOCTEST_SUBCASE("without path dict")
		{
			get_log_params.withPathsDict = false;
		}

		DOCTEST_SUBCASE("with path dict")
		{
			get_log_params.withPathsDict = true;
		}

		shv::core::utils::ShvLogRpcValueReader entries(getLog(readers, get_log_params, RpcValue::DateTime::fromIsoString("2024-07-07T18:06:20.850")));
		REQUIRE(entries.logHeader().withPathsDict() == get_log_params.withPathsDict);
		REQUIRE(as_vector(entries).size() == 7); // Verify all entries were read correctly
	}

	DOCTEST_SUBCASE("withSnapshot")
	{
		std::vector<std::function<shv::core::utils::ShvJournalFileReader()>> readers {
			create_reader({
				make_entry("2022-07-07T18:06:17.784Z", "value1", 0, true),
				make_entry("2022-07-07T18:06:17.784Z", "value2", 1, true),
				make_entry("2022-07-07T18:06:17.784Z", "value3", 3, true),
				make_entry("2022-07-07T18:06:17.800Z", "value3", 200, false),
				make_entry("2022-07-07T18:06:17.950Z", "value2", 10, false),
			}),
		};

		std::vector<shv::core::utils::ShvJournalEntry> expected_entries;
		bool expected_with_snapshot;

		DOCTEST_SUBCASE("without snapshot")
		{
			get_log_params.withSnapshot = false;
			expected_with_snapshot = false;

			DOCTEST_SUBCASE("without since")
			{
				expected_entries = {
					make_entry("2022-07-07T18:06:17.784Z", "value1", 0, true),
					make_entry("2022-07-07T18:06:17.784Z", "value2", 1, true),
					make_entry("2022-07-07T18:06:17.784Z", "value3", 3, true),
					make_entry("2022-07-07T18:06:17.800Z", "value3", 200, false),
					make_entry("2022-07-07T18:06:17.950Z", "value2", 10, false),
				};
			}

			DOCTEST_SUBCASE("with since")
			{
				get_log_params.since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.800");
				expected_entries = {
					make_entry("2022-07-07T18:06:17.950Z", "value2", 10, false),
				};
			}
		}

		DOCTEST_SUBCASE("with snapshot")
		{
			get_log_params.withSnapshot = true;
			DOCTEST_SUBCASE("without since param")
			{
				expected_with_snapshot = false;
				expected_entries = {
					make_entry("2022-07-07T18:06:17.784Z", "value1", 0, true),
					make_entry("2022-07-07T18:06:17.784Z", "value2", 1, true),
					make_entry("2022-07-07T18:06:17.784Z", "value3", 3, true),
					make_entry("2022-07-07T18:06:17.800Z", "value3", 200, false),
					make_entry("2022-07-07T18:06:17.950Z", "value2", 10, false),
				};
			}

			DOCTEST_SUBCASE("with since param")
			{
				expected_with_snapshot = true;
				DOCTEST_SUBCASE("one entry between snapshot and since")
				{
					get_log_params.since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.850");
					expected_entries = {
						make_entry("2022-07-07T18:06:17.850Z", "value1", 0, true),
						make_entry("2022-07-07T18:06:17.850Z", "value2", 1, true),
						make_entry("2022-07-07T18:06:17.850Z", "value3", 200, true),
						make_entry("2022-07-07T18:06:17.950Z", "value2", 10, false),
					};

				}

				DOCTEST_SUBCASE("one entry exactly on since")
				{
					// The entry won't be a part of the snapshot.
					get_log_params.since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.800");
					expected_entries = {
						make_entry("2022-07-07T18:06:17.800Z", "value1", 0, true),
						make_entry("2022-07-07T18:06:17.800Z", "value2", 1, true),
						make_entry("2022-07-07T18:06:17.800Z", "value3", 200, true),
						make_entry("2022-07-07T18:06:17.950", "value2", 10, false),
					};
				}
			}

			DOCTEST_SUBCASE("with record cound limit smaller than the snapshot")
			{
				expected_with_snapshot = true;
				get_log_params.since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.800");
				get_log_params.recordCountLimit = 1;
				// The whole snapshot should be sent regardless of the small recordCountLimit.
				expected_entries = {
					make_entry("2022-07-07T18:06:17.800Z", "value1", 0, true),
					make_entry("2022-07-07T18:06:17.800Z", "value2", 1, true),
					make_entry("2022-07-07T18:06:17.800Z", "value3", 200, true),
				};
			}

			DOCTEST_SUBCASE("with since after the last entry")
			{
				expected_with_snapshot = true;
				get_log_params.since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:20.850");
				expected_entries = {
					make_entry("2022-07-07T18:06:20.850Z", "value1", 0, true),
					make_entry("2022-07-07T18:06:20.850Z", "value2", 10, true),
					make_entry("2022-07-07T18:06:20.850Z", "value3", 200, true),
				};
			}
		}

		shv::core::utils::ShvLogRpcValueReader entries(getLog(readers, get_log_params, RpcValue::DateTime::fromIsoString("2024-07-07T18:06:20.850")));
		REQUIRE(entries.logHeader().withSnapShot() == expected_with_snapshot);
		REQUIRE(as_vector(entries) == expected_entries);
	}

	DOCTEST_SUBCASE("result since/until")
	{
		std::vector<std::function<shv::core::utils::ShvJournalFileReader()>> readers;
		RpcValue expected_since;
		RpcValue expected_until;

		DOCTEST_SUBCASE("empty log")
		{
			readers = {};

			DOCTEST_SUBCASE("since - not set, until - not set")
			{
				expected_since = RpcValue();
				expected_until = RpcValue();
			}

			DOCTEST_SUBCASE("since - set, until - not set")
			{
				auto since_param = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:15.557Z");
				get_log_params.since = since_param;
				expected_since = since_param;
				expected_until = since_param;
			}

			DOCTEST_SUBCASE("since - not set, until - set")
			{
				auto until_param = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:15.557Z");
				get_log_params.until = until_param;
				expected_since = until_param;
				expected_until = until_param;
			}

			DOCTEST_SUBCASE("since - set, until - set")
			{
				auto since_param = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:15.557Z");
				auto until_param = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:20.000Z");
				get_log_params.since = since_param;
				get_log_params.until = until_param;
				expected_since = since_param;
				expected_until = until_param;
			}
		}

		DOCTEST_SUBCASE("non-empty log")
		{
			readers = {
				create_reader({
					make_entry("2022-07-07T18:06:14.000Z", "value1", 10, false),
					make_entry("2022-07-07T18:06:15.557Z", "value2", 20, false),
					make_entry("2022-07-07T18:06:16.600Z", "value3", 30, false),
					make_entry("2022-07-07T18:06:17.784Z", "value4", 40, false),
				})
			};

			DOCTEST_SUBCASE("since - not set, until - not set")
			{
				expected_since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:14.000Z");
				expected_until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.784Z");
			}

			DOCTEST_SUBCASE("since - set, until - not set")
			{
				RpcValue::DateTime since_param;
				DOCTEST_SUBCASE("since on the first entry")
				{
					since_param = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:15.557Z");
					expected_since = since_param;
					expected_until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.784Z");
				}

				DOCTEST_SUBCASE("since NOT on the first entry")
				{
					DOCTEST_SUBCASE("before first entry")
					{
						since_param = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:13.000Z");
						expected_since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:14.000Z");
						expected_until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.784Z");
					}

					DOCTEST_SUBCASE("after first entry")
					{
						since_param = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:15.553Z");
						expected_since = since_param;
						expected_until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.784Z");
					}
				}

				get_log_params.since = since_param;
			}

			DOCTEST_SUBCASE("since - not set, until - set")
			{
				auto until_param = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:18.700");
				get_log_params.until = until_param;
				expected_since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:14.000Z");

				DOCTEST_SUBCASE("record count limit not hit")
				{
					expected_until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.784Z");
				}

				DOCTEST_SUBCASE("record count limit hit")
				{
					get_log_params.recordCountLimit = 2;
					expected_until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:16.600Z");
				}
			}

			DOCTEST_SUBCASE("since - set, until - set")
			{
				auto since_param = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:10.000Z");
				auto until_param = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:20.000Z");
				get_log_params.since = since_param;
				get_log_params.until = until_param;
				expected_since = since_param;

				DOCTEST_SUBCASE("record count limit not hit")
				{
					expected_since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:14.000Z");
					expected_until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.784Z");
				}

				DOCTEST_SUBCASE("record count limit hit")
				{
					get_log_params.recordCountLimit = 2;
					expected_since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:14.000Z");
					expected_until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:16.600Z");
				}
			}
		}

		auto log = shv::core::utils::ShvLogRpcValueReader(shv::core::utils::getLog(readers, get_log_params, RpcValue::DateTime::fromIsoString("2024-07-07T18:06:20.850")));
		REQUIRE(log.logHeader().sinceCRef() == expected_since);
		REQUIRE(log.logHeader().untilCRef() == expected_until);
	}

	DOCTEST_SUBCASE("miscellaneous result metadata")
	{
		std::vector<std::function<shv::core::utils::ShvJournalFileReader()>> readers = {
			create_reader({
				make_entry("2022-07-07T18:06:15.557Z", "value1", 10, false),
				make_entry("2022-07-07T18:06:16.600Z", "value2", 20, false),
				make_entry("2022-07-07T18:06:17.784Z", "value3", 30, false),
			})
		};
		get_log_params.recordCountLimit = 10;
		shv::core::utils::ShvLogRpcValueReader entries(getLog(readers, get_log_params, RpcValue::DateTime::fromIsoString("2024-07-07T18:06:20.850")));
		REQUIRE(entries.logHeader().dateTimeCRef().isValid());
		REQUIRE(entries.logHeader().logParamsCRef().recordCountLimit == 10);
		REQUIRE(entries.logHeader().recordCount() == 3);
	}

	DOCTEST_SUBCASE("sinceLast")
	{
		std::vector<std::function<shv::core::utils::ShvJournalFileReader()>> readers {
			create_reader({
				make_entry("2022-07-07T18:06:17.784Z", "value1", 0, true),
				make_entry("2022-07-07T18:06:17.784Z", "value2", 1, true),
				make_entry("2022-07-07T18:06:17.784Z", "value3", 3, true),
				make_entry("2022-07-07T18:06:17.800Z", "value3", 200, false),
				make_entry("2022-07-07T18:06:17.950Z", "value2", 10, false),
			}),
		};
		RpcValue expected_since;
		RpcValue expected_until;
		int expected_record_count;
		get_log_params.since = "last";

		DOCTEST_SUBCASE("withSnapshot == true")
		{
			get_log_params.withSnapshot = true;
			expected_since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.950Z");;
			expected_until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.950Z");
			expected_record_count = 3;
		}

		DOCTEST_SUBCASE("withSnapshot == false")
		{
			get_log_params.withSnapshot = false;
			expected_since = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.950Z");;
			expected_until = RpcValue::DateTime::fromIsoString("2022-07-07T18:06:17.950Z");
			expected_record_count = 1;
		}

		auto log = shv::core::utils::ShvLogRpcValueReader(shv::core::utils::getLog(readers, get_log_params, RpcValue::DateTime::fromIsoString("2024-07-07T18:06:20.850")));
		REQUIRE(log.logHeader().recordCount() == expected_record_count);
		REQUIRE(log.logHeader().sinceCRef() == expected_since);
		REQUIRE(log.logHeader().untilCRef() == expected_until);
	}
}

DOCTEST_TEST_CASE("newestMatchingFileIt")
{
	DOCTEST_SUBCASE("no timestamps")
	{
		std::vector<int64_t> timestamps;
		shv::core::utils::ShvGetLogParams params;
		REQUIRE(shv::core::utils::newestMatchingFileIt(timestamps, params) == timestamps.end());
	}

	DOCTEST_SUBCASE("some timestamps")
	{
		std::vector<int64_t> timestamps {
			10, 20, 30, 40, 50
		};

		int64_t expected_timestamp;
		shv::core::utils::ShvGetLogParams params;

		DOCTEST_SUBCASE("no since")
		{
			expected_timestamp = 10;
		}

		DOCTEST_SUBCASE("since before first")
		{
			params.since = RpcValue::DateTime::fromMSecsSinceEpoch(5);
			expected_timestamp = 10;
		}

		DOCTEST_SUBCASE("since between two timestamps")
		{
			params.since = RpcValue::DateTime::fromMSecsSinceEpoch(25);
			expected_timestamp = 20;
		}

		DOCTEST_SUBCASE("since on the last")
		{
			params.since = RpcValue::DateTime::fromMSecsSinceEpoch(50);
			expected_timestamp = 50;
		}

		DOCTEST_SUBCASE("since after last")
		{
			// Last file could have at least some of data for our since param.
			params.since = RpcValue::DateTime::fromMSecsSinceEpoch(60);
			expected_timestamp = 50;
		}

		DOCTEST_SUBCASE("since == last")
		{
			params.since = "last";
			expected_timestamp = 50;
		}

		auto it = shv::core::utils::newestMatchingFileIt(timestamps, params);
		REQUIRE(it != timestamps.end());
		REQUIRE(*it == expected_timestamp);
	}
}
