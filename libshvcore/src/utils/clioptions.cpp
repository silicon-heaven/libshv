#include <shv/core/utils/clioptions.h>

#include <shv/core/log.h>
#include <shv/core/string.h>
#include <shv/core/exception.h>

#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcvalue.h>

#include <cassert>
#include <filesystem>
#include <limits>
#include <algorithm>
#include <iostream>
#include <fstream>

using namespace shv::chainpack;
using namespace std::string_literals;
using namespace std;

namespace shv::core::utils {

CLIOptions::Option::Data::Data(chainpack::RpcValue::Type type_)
	: type(type_)
	, mandatory(false)
{
}

bool CLIOptions::Option::isValid() const
{
	return m_data.type != chainpack::RpcValue::Type::Invalid;
}

CLIOptions::Option& CLIOptions::Option::setNames(const StringList &names)
{
	m_data.names = names; return *this;
}

CLIOptions::Option& CLIOptions::Option::setNames(const std::string &name)
{
	m_data.names = StringList{name}; return *this;
}

CLIOptions::Option& CLIOptions::Option::setNames(const std::string &name1, const std::string &name2)
{
	m_data.names = StringList{name1, name2}; return *this;
}

const CLIOptions::StringList& CLIOptions::Option::names() const
{
	return m_data.names;
}

CLIOptions::Option& CLIOptions::Option::setType(chainpack::RpcValue::Type type)
{
	m_data.type = type; return *this;
}

chainpack::RpcValue::Type CLIOptions::Option::type() const
{
	return m_data.type;
}

CLIOptions::Option& CLIOptions::Option::setValue(const chainpack::RpcValue &val)
{
	m_data.value = val; return *this;
}

chainpack::RpcValue CLIOptions::Option::value() const
{
	return m_data.value;
}

CLIOptions::Option& CLIOptions::Option::setDefaultValue(const chainpack::RpcValue &val)
{
	m_data.defaultValue = val; return *this;
}

chainpack::RpcValue CLIOptions::Option::defaultValue() const
{
	return m_data.defaultValue;
}

CLIOptions::Option& CLIOptions::Option::setComment(const std::string &s)
{
	m_data.comment = s; return *this;
}

const std::string& CLIOptions::Option::comment() const
{
	return m_data.comment;
}

CLIOptions::Option& CLIOptions::Option::setMandatory(bool b)
{
	m_data.mandatory = b; return *this;
}

bool CLIOptions::Option::isMandatory() const
{
	return m_data.mandatory;
}

bool CLIOptions::Option::isSet() const
{
	return value().isValid();
}

CLIOptions::Option& CLIOptions::Option::setValueString(const std::string &val_str)
{
	RpcValue::Type t = type();
	switch(t) {
	case(RpcValue::Type::Invalid):
		shvWarning() << "Setting value:" << val_str << "to an invalid type option.";
		break;
	case(RpcValue::Type::Bool):
	{
		if(val_str.empty()) {
			setValue(true);
		}
		else {
			bool ok;
			int n = String::toInt(val_str, &ok);
			if(ok) {
				setValue(n != 0);
			}
			else {
				bool is_true = true;
				for(const char * const s : {"n", "no", "false"}) {
					if(String::equal(val_str, s, String::CaseInsensitive)) {
						is_true = false;
						break;
					}
				}
				setValue(is_true);
			}
		}
		break;
	}
	case RpcValue::Type::Int:
	case RpcValue::Type::UInt:
	{
		bool ok;
		setValue(String::toInt(val_str, &ok));
		if(!ok)
			shvWarning() << "Value:" << val_str << "cannot be converted to Int.";
		break;
	}
	case(RpcValue::Type::Double):
	{
		bool ok;
		setValue(String::toDouble(val_str, &ok));
		if(!ok)
			shvWarning() << "Value:" << val_str << "cannot be converted to Double.";
		break;
	}
	default:
		setValue(val_str);
	}
	return *this;
}

CLIOptions::CLIOptions()
{
	addOption("abortOnException").setType(RpcValue::Type::Bool).setNames("--abort-on-exception").setComment("Abort application on exception");
	addOption("help").setType(RpcValue::Type::Bool).setNames("-h", "--help").setComment("Print help");
}

CLIOptions::~CLIOptions() = default;

CLIOptions::Option& CLIOptions::addOption(const std::string &key, const CLIOptions::Option& opt)
{
	m_options[key] = opt;
	return m_options[key];
}

bool CLIOptions::removeOption(const std::string &key)
{
	return m_options.erase(key) > 0;
}

const CLIOptions::Option& CLIOptions::option(const std::string& name, bool throw_exc) const
{
	auto it = m_options.find(name);
	if(it == m_options.end()) {
		if(throw_exc) {
			std::string msg = "Key '" + name + "' not found.";
			shvWarning() << msg;
			SHV_EXCEPTION(msg);
		}
		static Option so;
		return so;
	}
	return m_options.at(name);
}

CLIOptions::Option& CLIOptions::optionRef(const std::string& name)
{
	auto it = m_options.find(name);
	if(it == m_options.end()) {
		std::string msg = "Key '" + name + "' not found.";
		shvWarning() << msg;
		SHV_EXCEPTION(msg);
	}
	return m_options[name];
}

const std::map<std::string, CLIOptions::Option>& CLIOptions::options() const
{
	return m_options;
}

RpcValue::Map CLIOptions::values() const
{
	RpcValue::Map ret;
	for(const auto &kv : m_options)
		ret[kv.first] = value(kv.first);
	return ret;
}

RpcValue CLIOptions::value(const std::string &name) const
{
	RpcValue ret = value_helper(name, shv::core::Exception::Throw);
	return ret;
}

RpcValue CLIOptions::value(const std::string& name, const RpcValue default_value) const
{
	RpcValue ret = value_helper(name, !shv::core::Exception::Throw);
	if(!ret.isValid())
		ret = default_value;
	return ret;
}

bool CLIOptions::isValueSet(const std::string &name) const
{
	return option(name, !shv::core::Exception::Throw).isSet();
}

RpcValue CLIOptions::value_helper(const std::string &name, bool throw_exception) const
{
	Option opt = option(name, throw_exception);
	if(!opt.isValid())
		return RpcValue();
	RpcValue ret = opt.value();
	if(!ret.isValid())
		ret = opt.defaultValue();
	if(!ret.isValid())
		ret = RpcValue::fromType(opt.type());
	return ret;
}

bool CLIOptions::optionExists(const std::string &name) const
{
	return option(name, !shv::core::Exception::Throw).isValid();
}

bool CLIOptions::setValue(const std::string& name, const RpcValue val, bool throw_exc)
{
	Option o = option(name, false);
	if(optionExists(name)) {
		Option &orf = optionRef(name);
		orf.setValue(val);
		return true;
	}

	std::string msg = "setValue():" + val.toCpon() + " Key '" + name + "' not found.";
	shvWarning() << msg;
	if(throw_exc) {
		SHV_EXCEPTION(msg);
	}
	return false;

}

std::string CLIOptions::takeArg(bool &ok)
{
	ok = m_parsedArgIndex < m_arguments.size();
	if(ok)
		return m_arguments.at(m_parsedArgIndex++);
	return std::string();
}

std::string CLIOptions::peekArg(bool &ok) const
{
	ok = m_parsedArgIndex < m_arguments.size();
	if(ok)
		return m_arguments.at(m_parsedArgIndex);
	return std::string();
}

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
void CLIOptions::parse(int argc, char* argv[])
{
	StringList args;
	for(int i=0; i<argc; i++)
		args.emplace_back(argv[i]);
	parse(args);
}

void CLIOptions::parse(const StringList& cmd_line_args)
{
	m_isAppBreak = false;
	m_parsedArgIndex = 0;
	m_arguments = StringList(cmd_line_args.begin()+1, cmd_line_args.end());
	m_unusedArguments = StringList();
	m_parseErrors = StringList();
	m_allArgs = cmd_line_args;

	bool ok;
	while(true) {
		std::string arg = takeArg(ok);
		if(!ok) {
			break;
		}
		if(arg == "--help" || arg == "-h") {
			setHelp(true);
			m_isAppBreak = true;
			return;
		}
		bool found = false;
		for(auto &kv : m_options) {
			Option &opt = kv.second;
			StringList names = opt.names();
			if(std::find(names.begin(), names.end(), arg) != names.end()) {
				found = true;
				arg = peekArg(ok);
				if(!ok) {
					// switch has no value entered
					arg = std::string();
				}
				else if((!arg.empty() && arg[0] == '-')) {
					// might be negative number or next switch
					if(opt.type() != RpcValue::Type::Int)
						arg = std::string();
				}
				else {
					arg = takeArg(ok);
				}
				opt.setValueString(arg);
				break;
			}
		}
		if(!found) {
			if(!arg.empty() && arg[0] == '-')
				m_unusedArguments.push_back(arg);
		}
	}
	{
		for(const auto &kv : m_options) {
			const Option &opt = kv.second;
			if(opt.isMandatory() && !opt.value().isValid() && !opt.names().empty()) {
				addParseError("Mandatory option '" + opt.names()[0] + "' not set.");
			}
		}
	}
	shv::core::Exception::setAbortOnException(isAbortOnException());
}

bool CLIOptions::isParseError() const
{
	return !m_parseErrors.empty();
}

bool CLIOptions::isAppBreak() const
{
	return m_isAppBreak;
}

CLIOptions::StringList CLIOptions::parseErrors() const
{
	return m_parseErrors;
}

CLIOptions::StringList CLIOptions::unusedArguments()
{
	return m_unusedArguments;
}

std::tuple<std::string, std::string> CLIOptions::applicationDirAndName() const
{
	static std::string app_dir;
	static std::string app_name;
	if(app_name.empty()) {
		if(!m_allArgs.empty()) {
			std::string app_file_path = m_allArgs[0];
			char sep = '/';
			size_t sep_pos = app_file_path.find_last_of(sep);
			if(sep_pos == std::string::npos) {
				app_name = app_file_path;
			}
			else {
				app_name = app_file_path.substr(sep_pos + 1);
				app_dir = app_file_path.substr(0, sep_pos);
			}
	#ifdef _WIN32
			std::string ext = ".exe";
	#else
			std::string ext = ".so";
	#endif
			if(app_name.size() > ext.size()) {
				std::string app_ext = app_name.substr(app_name.size() - ext.size());
				if(String::equal(ext, app_ext, String::CaseInsensitive))
					app_name = app_name.substr(0, app_name.size() - ext.size());
			}
		}
	}
	std::replace(app_dir.begin(), app_dir.end(), '\\', '/');
	return std::tuple<std::string, std::string>(app_dir, app_name);
}

std::string CLIOptions::applicationDir() const
{
	return std::get<0>(applicationDirAndName());
}

std::string CLIOptions::applicationName() const
{
	return std::get<1>(applicationDirAndName());
}

void CLIOptions::printHelp(std::ostream &os) const
{
	using namespace std;
	os << applicationName() << " [OPTIONS]" << endl << endl;
	os << "OPTIONS:" << endl << endl;
	for(const auto &kv : m_options) {
		const Option &opt = kv.second;
		os << String::join(opt.names(), ", ");
		if(opt.type() != RpcValue::Type::Bool) {
			if(opt.type() == RpcValue::Type::Int
					|| opt.type() == RpcValue::Type::UInt
					|| opt.type() == RpcValue::Type::Double) os << " " << "number";
			else os << " " << "'string'";
		}
		RpcValue def_val = opt.defaultValue();
		if(def_val.isValid())
			os << " DEFAULT=" << def_val.toStdString();
		if(opt.isMandatory())
			os << " MANDATORY";
		os << endl;
		const std::string& oc = opt.comment();
		if(!oc.empty())
			os << "\t" << oc << endl;
	}
	os << NecroLog::cliHelp() << std::endl;
	std::string topics = NecroLog::registeredTopicsInfo();
	if(!topics.empty())
		std::cout << topics << std::endl;
}

void CLIOptions::printHelp() const
{
	printHelp(std::cout);
}

void CLIOptions::dump(std::ostream &os) const
{
	for(const auto &kv : m_options) {
		const Option &opt = kv.second;
		os << kv.first << '(' << String::join(opt.names(), ", ") << ')' << ": " << opt.value().asString() << std::endl;
	}
}

void CLIOptions::dump() const
{
	using namespace std;
	std::cout<< "=============== options values dump ==============" << endl;
	dump(std::cout);
	std::cout << "-------------------------------------------------" << endl;
}

void CLIOptions::addParseError(const std::string& err)
{
	m_parseErrors.push_back(err);
}

ConfigCLIOptions::ConfigCLIOptions()
{
	addOption("config").setType(RpcValue::Type::String).setNames("--config").setComment("Application config name, it is loaded from {config}[.conf] if file exists in {config-dir}, deault value is {app-name}.conf");
	addOption("configDir").setType(RpcValue::Type::String).setNames("--config-dir").setComment("Directory where application config fiels are searched, default value: {app-dir-path}.");
}

void ConfigCLIOptions::parse(const StringList &cmd_line_args)
{
	Super::parse(cmd_line_args);
}

bool ConfigCLIOptions::loadConfigFile()
{
	std::string config_file = configFile();
	std::ifstream fis(config_file);
	shvInfo() << "Checking presence of config file:" << config_file;
	if(fis) {
		shvInfo() << "Reading config file:" << config_file;
		shv::chainpack::RpcValue rv;
		shv::chainpack::CponReader rd(fis);
		std::string err;
		rv = rd.read(&err);
		if(err.empty()) {
			mergeConfig(rv);
		}
		else {
			shvError() << "Error parsing config file:" << config_file << err;
			return false;
		}
	}
	else {
		shvError() << "Config file:" << config_file << "not found.";
	}
	return true;
}
namespace {
bool is_absolute_path(const std::string &path)
{
#ifdef _WIN32
	return path.size() > 2 && path[1] == ':';
#else
	return !path.empty() && path[0] == '/';
#endif
}
}
std::string ConfigCLIOptions::configFile() const
{
	auto [abs_config_dir, abs_config_file] = absoluteConfigPaths(configDir(), config());
	return abs_config_file;
}

std::string ConfigCLIOptions::effectiveConfigDir() const
{
	auto [abs_config_dir, abs_config_file] = absoluteConfigPaths(configDir(), config());
	return abs_config_dir;
}

std::tuple<std::string, std::string> ConfigCLIOptions::absoluteConfigPaths(const std::string &config_dir, const std::string &config_file) const
{
	using shv::core::utils::joinPath;
	static const auto conf_ext = ".conf"s;
	auto cwd = std::filesystem::current_path().string();
	auto default_config_name = applicationName() + conf_ext;
	if(config_file.empty()) {
		if(config_dir.empty())
			return make_tuple(cwd, joinPath(cwd, default_config_name));

		if(is_absolute_path(config_dir))
			return make_tuple(config_dir, joinPath(config_dir, default_config_name));

		return make_tuple(joinPath(cwd, config_dir), joinPath(cwd, config_dir, default_config_name));
	}

	if(is_absolute_path(config_file)) {
		if(config_dir.empty()) {
			auto sep_pos = config_file.find_last_of('/');
			// absolute config file must contain '/'
			return make_tuple(config_file.substr(0, sep_pos), config_file);
		}

		if(is_absolute_path(config_dir))
			return make_tuple(config_dir, config_file);

		return make_tuple(joinPath(cwd, config_dir), config_file);
	}
	/* relative config_file */
	if(config_dir.empty())
		return make_tuple(cwd, joinPath(cwd, config_file));

	if(is_absolute_path(config_dir))
		return make_tuple(config_dir, joinPath(config_dir, config_file));

	return make_tuple(joinPath(cwd, config_dir), joinPath(cwd, config_dir, config_file));
}

void ConfigCLIOptions::mergeConfig_helper(const std::string &key_prefix, const shv::chainpack::RpcValue &config_map)
{
	const chainpack::RpcValue::Map &cm = config_map.asMap();
	for(const auto &kv : cm) {
		std::string key = kv.first;
		if(!key_prefix.empty())
			key = key_prefix + '.' + key;
		chainpack::RpcValue rv = kv.second;
		if(options().find(key) != options().end()) {
			Option &opt = optionRef(key);
			if(!opt.isSet()) {
				opt.setValue(rv);
			}
		}
		else if(rv.isMap()) {
			mergeConfig_helper(key, rv);
		}
		else {
			shvWarning() << "Cannot merge nonexisting option key:" << key;
		}
	}
}

void ConfigCLIOptions::mergeConfig(const chainpack::RpcValue &config_map)
{
	mergeConfig_helper(std::string(), config_map);
}

}
