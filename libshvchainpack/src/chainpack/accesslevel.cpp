#include <shv/chainpack/accesslevel.h>
#include <shv/chainpack/rpc.h>

namespace shv::chainpack {

const char *accessLevelToAccessString(AccessLevel access_level)
{
	switch(access_level) {
	case AccessLevel::Browse: return shv::chainpack::Rpc::ROLE_BROWSE;
	case AccessLevel::Read: return shv::chainpack::Rpc::ROLE_READ;
	case AccessLevel::Write: return shv::chainpack::Rpc::ROLE_WRITE;
	case AccessLevel::Command: return shv::chainpack::Rpc::ROLE_COMMAND;
	case AccessLevel::Config: return shv::chainpack::Rpc::ROLE_CONFIG;
	case AccessLevel::Service: return shv::chainpack::Rpc::ROLE_SERVICE;
	case AccessLevel::SuperService: return shv::chainpack::Rpc::ROLE_SUPER_SERVICE;
	case AccessLevel::Devel: return shv::chainpack::Rpc::ROLE_DEVEL;
	case AccessLevel::Admin: return shv::chainpack::Rpc::ROLE_ADMIN;
	default: return "";
	}
}

std::optional<AccessLevel> accessLevelFromAccessString(std::string_view s)
{
	if(s == Rpc::ROLE_BROWSE) return AccessLevel::Browse;
	if(s == Rpc::ROLE_READ) return AccessLevel::Read;
	if(s == Rpc::ROLE_WRITE) return AccessLevel::Write;
	if(s == Rpc::ROLE_COMMAND) return AccessLevel::Command;
	if(s == Rpc::ROLE_CONFIG) return AccessLevel::Config;
	if(s == Rpc::ROLE_SERVICE) return AccessLevel::Service;
	if(s == Rpc::ROLE_SUPER_SERVICE) return AccessLevel::SuperService;
	if(s == Rpc::ROLE_DEVEL) return AccessLevel::Devel;
	if(s == Rpc::ROLE_ADMIN) return AccessLevel::Admin;

	return {};
}

std::optional<AccessLevel> accessLevelFromInt(int i)
{
	switch(static_cast<AccessLevel>(i)) {
	case AccessLevel::None: return AccessLevel::None;

	case AccessLevel::Browse: return AccessLevel::Browse;
	case AccessLevel::Browse1: return AccessLevel::Browse1;
	case AccessLevel::Browse2: return AccessLevel::Browse2;
	case AccessLevel::Browse3: return AccessLevel::Browse3;
	case AccessLevel::Browse4: return AccessLevel::Browse4;
	case AccessLevel::Browse5: return AccessLevel::Browse5;
	case AccessLevel::Browse6: return AccessLevel::Browse6;

	case AccessLevel::Read: return AccessLevel::Read;
	case AccessLevel::Read1: return AccessLevel::Read1;
	case AccessLevel::Read2: return AccessLevel::Read2;
	case AccessLevel::Read3: return AccessLevel::Read3;
	case AccessLevel::Read4: return AccessLevel::Read4;
	case AccessLevel::Read5: return AccessLevel::Read5;
	case AccessLevel::Read6: return AccessLevel::Read6;
	case AccessLevel::Read7: return AccessLevel::Read7;

	case AccessLevel::Write: return AccessLevel::Write;
	case AccessLevel::Write1: return AccessLevel::Write1;
	case AccessLevel::Write2: return AccessLevel::Write2;
	case AccessLevel::Write3: return AccessLevel::Write3;
	case AccessLevel::Write4: return AccessLevel::Write4;
	case AccessLevel::Write5: return AccessLevel::Write5;
	case AccessLevel::Write6: return AccessLevel::Write6;
	case AccessLevel::Write7: return AccessLevel::Write7;

	case AccessLevel::Command: return AccessLevel::Command;
	case AccessLevel::Command1: return AccessLevel::Command1;
	case AccessLevel::Command2: return AccessLevel::Command2;
	case AccessLevel::Command3: return AccessLevel::Command3;
	case AccessLevel::Command4: return AccessLevel::Command4;
	case AccessLevel::Command5: return AccessLevel::Command5;
	case AccessLevel::Command6: return AccessLevel::Command6;
	case AccessLevel::Command7: return AccessLevel::Command7;

	case AccessLevel::Config: return AccessLevel::Config;
	case AccessLevel::Config1: return AccessLevel::Config1;
	case AccessLevel::Config2: return AccessLevel::Config2;
	case AccessLevel::Config3: return AccessLevel::Config3;
	case AccessLevel::Config4: return AccessLevel::Config4;
	case AccessLevel::Config5: return AccessLevel::Config5;
	case AccessLevel::Config6: return AccessLevel::Config6;
	case AccessLevel::Config7: return AccessLevel::Config7;

	case AccessLevel::Service: return AccessLevel::Service;
	case AccessLevel::Service1: return AccessLevel::Service1;
	case AccessLevel::Service2: return AccessLevel::Service2;
	case AccessLevel::Service3: return AccessLevel::Service3;
	case AccessLevel::Service4: return AccessLevel::Service4;
	case AccessLevel::Service5: return AccessLevel::Service5;
	case AccessLevel::Service6: return AccessLevel::Service6;
	case AccessLevel::Service7: return AccessLevel::Service7;

	case AccessLevel::SuperService: return AccessLevel::SuperService;
	case AccessLevel::SuperService1: return AccessLevel::SuperService1;
	case AccessLevel::SuperService2: return AccessLevel::SuperService2;
	case AccessLevel::SuperService3: return AccessLevel::SuperService3;
	case AccessLevel::SuperService4: return AccessLevel::SuperService4;
	case AccessLevel::SuperService5: return AccessLevel::SuperService5;
	case AccessLevel::SuperService6: return AccessLevel::SuperService6;
	case AccessLevel::SuperService7: return AccessLevel::SuperService7;

	case AccessLevel::Devel: return AccessLevel::Devel;
	case AccessLevel::Devel1: return AccessLevel::Devel1;
	case AccessLevel::Devel2: return AccessLevel::Devel2;
	case AccessLevel::Devel3: return AccessLevel::Devel3;
	case AccessLevel::Devel4: return AccessLevel::Devel4;
	case AccessLevel::Devel5: return AccessLevel::Devel5;
	case AccessLevel::Devel6: return AccessLevel::Devel6;

	case AccessLevel::Admin: return AccessLevel::Admin;
	}
	return {};
}

}
