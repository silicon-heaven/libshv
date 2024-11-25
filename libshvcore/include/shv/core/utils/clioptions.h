#pragma once

#include <shv/core/shvcoreglobal.h>

#include <shv/chainpack/rpcvalue.h>

#define CLIOPTION_QUOTE_ME(x) #x

#define CLIOPTION_GETTER_SETTER(ptype, getter_prefix, setter_prefix, name_rest) \
	CLIOPTION_GETTER_SETTER2(ptype, CLIOPTION_QUOTE_ME(getter_prefix##name_rest), getter_prefix, setter_prefix, name_rest)

#define CLIOPTION_GETTER_SETTER2(ptype, pkey, getter_prefix, setter_prefix, name_rest) \
	public: ptype getter_prefix##name_rest() const { \
		shv::chainpack::RpcValue val = value(pkey); \
		return val.to<ptype>(); \
	} \
	protected: shv::core::utils::CLIOptions::Option& getter_prefix##name_rest##_optionRef() {return optionRef(pkey);} \
	public: bool getter_prefix##name_rest##_isset() const {return isValueSet(pkey);} \
	public: bool setter_prefix##name_rest(const ptype &val) {return setValue(pkey, val);}

namespace shv {
namespace chainpack { class RpcValue; }
namespace core::utils {

class SHVCORE_DECL_EXPORT CLIOptions
{
public:
	CLIOptions();
	virtual ~CLIOptions();

	using StringList = std::vector<std::string>;
public:
	class SHVCORE_DECL_EXPORT Option
	{
	private:
		struct SHVCORE_DECL_EXPORT Data
		{
			chainpack::RpcValue::Type type = chainpack::RpcValue::Type::Invalid;
			StringList names;
			chainpack::RpcValue value;
			chainpack::RpcValue defaultValue;
			std::string comment;
			bool mandatory = false;

			Data(chainpack::RpcValue::Type type_ = chainpack::RpcValue::Type::Invalid);
		};
		Data m_data;
	public:
		bool isValid() const;

		Option& setNames(const StringList &names);
		Option& setNames(const std::string &name);
		Option& setNames(const std::string &name1, const std::string &name2);
		const StringList& names() const;
		Option& setType(chainpack::RpcValue::Type type);
		chainpack::RpcValue::Type type() const;
		Option& setValueString(const std::string &val_str);
		Option& setValue(const chainpack::RpcValue &val);
		chainpack::RpcValue value() const;
		Option& setDefaultValue(const chainpack::RpcValue &val);
		chainpack::RpcValue defaultValue() const;
		Option& setComment(const std::string &s);
		const std::string& comment() const;
		Option& setMandatory(bool b);
		bool isMandatory() const;
		bool isSet() const;
	public:
		Option() = default;
	};
public:
	CLIOPTION_GETTER_SETTER2(bool, "abortOnException", is, set, AbortOnException)
	CLIOPTION_GETTER_SETTER2(bool, "help", is, set, Help)

	Option& addOption(const std::string &key, const Option &opt = Option());
	bool removeOption(const std::string &key);
	const Option& option(const std::string &name, bool throw_exc = true) const;
	Option& optionRef(const std::string &name);
	const std::map<std::string, Option>& options() const;

	// NOLINTNEXTLINE(modernize-avoid-c-arrays)
	void parse(int argc, char *argv[]);
	virtual void parse(const StringList &cmd_line_args);
	bool isParseError() const;
	bool isAppBreak() const;
	StringList parseErrors() const;
	StringList unusedArguments();

	std::string applicationDir() const;
	std::string applicationName() const;
	void printHelp() const;
	void printHelp(std::ostream &os) const;
	void dump() const;
	void dump(std::ostream &os) const;

	bool optionExists(const std::string &name) const;
	chainpack::RpcValue::Map values() const;
	chainpack::RpcValue value(const std::string &name) const;
	chainpack::RpcValue value(const std::string &name, const chainpack::RpcValue& default_value) const;
	/// value is explicitly set from command line or in config file
	/// defaultValue is not considered to be an explicitly set value
	bool isValueSet(const std::string &name) const;
	bool setValue(const std::string &name, const chainpack::RpcValue& val, bool throw_exc = true);
protected:
	chainpack::RpcValue value_helper(const std::string &name, bool throw_exception) const;
	std::tuple<std::string, std::string> applicationDirAndName() const;
	std::string takeArg(bool &ok);
	std::string peekArg(bool &ok) const;
	void addParseError(const std::string &err);
private:
	std::map<std::string, Option> m_options;
	StringList m_arguments;
	size_t m_parsedArgIndex = 0;
	StringList m_unusedArguments;
	StringList m_parseErrors;
	bool m_isAppBreak = false;
	StringList m_allArgs;
};

class SHVCORE_DECL_EXPORT ConfigCLIOptions : public CLIOptions
{
private:
	typedef CLIOptions Super;
public:
	ConfigCLIOptions();
	~ConfigCLIOptions() override = default;

	CLIOPTION_GETTER_SETTER(std::string, c, setC, onfig)
	CLIOPTION_GETTER_SETTER(std::string, c, setC, onfigDir)

	void parse(const StringList &cmd_line_args) override;
	bool loadConfigFile();

	void mergeConfig(const shv::chainpack::RpcValue &config_map);
	std::string configFile() const;
	std::string effectiveConfigDir() const;
	std::tuple<std::string, std::string>absoluteConfigPaths(const std::string &config_dir, const std::string &config_file) const;
protected:
	void mergeConfig_helper(const std::string &key_prefix, const shv::chainpack::RpcValue &config_map);
};

}}

