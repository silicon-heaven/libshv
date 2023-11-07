#include "clioptions.h"
#include "../log.h"
#include "../string.h"

#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcvalue.h>
/*
#include <shv/core/log.h>
#include <shv/core/utils.h>
#include <shv/core/assert.h>
#include <shv/core/exception.h>

#include <std::stringBuilder>
#include <StringList>
#include <QDir>
#include <QJsonParseError>
#include <QTextStream>
*/
#ifdef Q_OS_WIN
#include <qt_windows.h> // needed by CLIOptions::applicationDirAndName()
#endif

#include <limits>
#include <algorithm>
#include <iostream>
#include <fstream>

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

CLIOptions::Option& CLIOptions::Option::setValueString(const std::string &val_str)
{
	cp::RpcValue::Type t = type();
	switch(t) {
	case(cp::RpcValue::Type::Invalid):
		shvWarning() << "Setting value:" << val_str << "to an invalid type option.";
		break;
	case(cp::RpcValue::Type::Bool):
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
	case cp::RpcValue::Type::Int:
	case cp::RpcValue::Type::UInt:
	{
		bool ok;
		setValue(String::toInt(val_str, &ok));
		if(!ok)
			shvWarning() << "Value:" << val_str << "cannot be converted to Int.";
		break;
	}
	case(cp::RpcValue::Type::Double):
	{
		bool ok;
		setValue(String::toDouble(val_str, &ok));
		if(!ok)
			shvWarning() << "Value:" << val_str << "cannot be converted to Double.";
		break;
	}
	default:
		setValue(val_str);
		//shvWarning() << val_str << "->" << names() << "->" << value();
	}
	return *this;
}

CLIOptions::CLIOptions()
{
	addOption("abortOnException").setType(cp::RpcValue::Type::Bool).setNames("--abort-on-exception").setComment("Abort application on exception");
	addOption("help").setType(cp::RpcValue::Type::Bool).setNames("-h", "--help").setComment("Print help");
}

CLIOptions::~CLIOptions()
{
}

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

cp::RpcValue::Map CLIOptions::values() const
{
	cp::RpcValue::Map ret;
	for(const auto &kv : m_options)
		ret[kv.first] = value(kv.first);
	return ret;
}

cp::RpcValue CLIOptions::value(const std::string &name) const
{
	cp::RpcValue ret = value_helper(name, shv::core::Exception::Throw);
	return ret;
}

cp::RpcValue CLIOptions::value(const std::string& name, const cp::RpcValue default_value) const
{
	cp::RpcValue ret = value_helper(name, !shv::core::Exception::Throw);
	if(!ret.isValid())
		ret = default_value;
	return ret;
}

bool CLIOptions::isValueSet(const std::string &name) const
{
	return option(name, !shv::core::Exception::Throw).isSet();
}

cp::RpcValue CLIOptions::value_helper(const std::string &name, bool throw_exception) const
{
	Option opt = option(name, throw_exception);
	if(!opt.isValid())
		return cp::RpcValue();
	cp::RpcValue ret = opt.value();
	if(!ret.isValid())
		ret = opt.defaultValue();
	if(!ret.isValid())
		ret = cp::RpcValue::fromType(opt.type());
	return ret;
}

bool CLIOptions::optionExists(const std::string &name) const
{
	return option(name, !shv::core::Exception::Throw).isValid();
}

bool CLIOptions::setValue(const std::string& name, const cp::RpcValue val, bool throw_exc)
{
	Option o = option(name, false);
	if(optionExists(name)) {
		Option &orf = optionRef(name);
		orf.setValue(val);
		return true;
	}
	else {
		std::string msg = "setValue():" + val.toCpon() + " Key '" + name + "' not found.";
		shvWarning() << msg;
		if(throw_exc) {
			SHV_EXCEPTION(msg);
		}
		return false;
	}
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

void CLIOptions::parse(int argc, char* argv[])
{
	StringList args;
	for(int i=0; i<argc; i++)
		args.push_back(argv[i]);
	parse(args);
}

void CLIOptions::parse(const StringList& cmd_line_args)
{
	//shvLogFuncFrame() << cmd_line_args;
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
			//addParseError("Unexpected empty argument.");
			break;
		}
		if(arg == "--help" || arg == "-h") {
			setHelp(true);
			m_isAppBreak = true;
			return;
		}
		else {
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
					else if((arg.size() && arg[0] == '-')) {
						// might be negative number or next switch
						if(opt.type() != cp::RpcValue::Type::Int)
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
				if(arg.size() && arg[0] == '-')
					m_unusedArguments.push_back(arg);
			}
		}
	}
	{
		for(const auto &kv : m_options) {
			const Option &opt = kv.second;
			//LOGDEB() << "option:" << it.key() << "is mandatory:" << opt.isMandatory() << "is valid:" << opt.value().isValid();
			if(opt.isMandatory() && !opt.value().isValid() && !opt.names().empty()) {
				addParseError("Mandatory option '" + opt.names()[0] + "' not set.");
			}
		}
	}
	shv::core::Exception::setAbortOnException(isAbortOnException());
}

std::tuple<std::string, std::string> CLIOptions::applicationDirAndName() const
{
	static std::string app_dir;
	static std::string app_name;
	if(app_name.empty()) {
		if(m_allArgs.size()) {
	#ifdef Q_OS_WIN
			std::string app_file_path;
			wchar_t buffer[MAX_PATH + 2];
			DWORD v = GetModuleFileName(0, buffer, MAX_PATH + 1);
			buffer[MAX_PATH + 1] = 0;
			if (v <= MAX_PATH)
				app_file_path = std::string::fromWCharArray(buffer);
			QChar sep = '\\';
	#else
			std::string app_file_path = m_allArgs[0];
			char sep = '/';
	#endif
			size_t sep_pos = app_file_path.find_last_of(sep);
			if(sep_pos == std::string::npos) {
				app_name = app_file_path;
			}
			else {
				app_name = app_file_path.substr(sep_pos + 1);
				app_dir = app_file_path.substr(0, sep_pos);
			}
			//shvInfo() << "app dir:" << app_dir << "name:" << app_name;
	#ifdef Q_OS_WIN
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
		if(opt.type() != cp::RpcValue::Type::Bool) {
			if(opt.type() == cp::RpcValue::Type::Int
					|| opt.type() == cp::RpcValue::Type::UInt
					|| opt.type() == cp::RpcValue::Type::Double) os << " " << "number";
			else os << " " << "'string'";
		}
		//os << ':';
		cp::RpcValue def_val = opt.defaultValue();
		if(def_val.isValid())
			os << " DEFAULT=" << def_val.toStdString();
		if(opt.isMandatory())
			os << " MANDATORY";
		os << endl;
		const std::string& oc = opt.comment();
		if(!oc.empty())
			os << "\t" << oc << endl;
	}
	//os << shv::core::ShvLog::logCLIHelp() << endl;
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
	addOption("config").setType(cp::RpcValue::Type::String).setNames("--config").setComment("Application config name, it is loaded from {config}[.conf] if file exists in {config-dir}, deault value is {app-name}.conf");
	addOption("configDir").setType(cp::RpcValue::Type::String).setNames("--config-dir").setComment("Directory where application config fiels are searched, default value: {app-dir-path}.");
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
		shvInfo() << "Config file:" << config_file << "not found.";
	}
	return true;
}

std::string ConfigCLIOptions::configFile()
{
	std::string config = "config";
	std::string conf_ext = ".conf";
	std::string config_file;
	if(isValueSet(config)) {
		config_file = value(config).toString();
		if(config_file.empty()) {
			/// explicitly set empty config means DO NOT load config from any file
			return std::string();
		}
	}
	else {
		config_file = applicationName() + conf_ext;
	}
#ifdef Q_OS_WIN
	bool is_absolute_path = config_file.size() > 2 && config_file[1] == ':';
#else
	bool is_absolute_path = config_file.size() && config_file[0] == '/';
#endif
	if(!is_absolute_path) {
		std::string config_dir = configDir();
		if(config_dir.empty())
			config_dir = applicationDir();
		config_file = config_dir + '/' + config_file;
	}
	if(!String::endsWith(config_file, conf_ext)) {
		std::ifstream f(config_file);
		if(!f)
			config_file += conf_ext;
	}
	return config_file;
}

std::string ConfigCLIOptions::effectiveConfigDir()
{
	if(!configDir().empty())
		return configDir();
	String cfg = config();
	if(!cfg.empty() && cfg[0] == '/') {
		auto ix = cfg.lastIndexOf('/');
		if(ix != std::string::npos) {
			return  cfg.mid(0, ix);
		}
	}
	return applicationDir();
}

void ConfigCLIOptions::mergeConfig_helper(const std::string &key_prefix, const shv::chainpack::RpcValue &config_map)
{
	//shvLogFuncFrame() << key_prefix;
	const chainpack::RpcValue::Map &cm = config_map.asMap();
	for(const auto &kv : cm) {
		std::string key = kv.first;
		//SHV_ASSERT(!key.empty(), "Empty key!", continue);
		if(!key_prefix.empty())
			key = key_prefix + '.' + key;
		chainpack::RpcValue rv = kv.second;
		if(options().find(key) != options().end()) {
			Option &opt = optionRef(key);
			if(!opt.isSet()) {
				//shvInfo() << key << "-->" << rv.toCpon();
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
	/*
	for(const auto &kv : options()) {
		std::string key = kv.first;
		shvInfo() << key << "-->" << option(key).value().toCpon();
	}
	*/
}

}}}
