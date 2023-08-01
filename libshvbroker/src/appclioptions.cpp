#include "appclioptions.h"

#include <shv/chainpack/irpcconnection.h>

namespace cp = shv::chainpack;

namespace shv::broker {

AppCliOptions::AppCliOptions()
{
	addOption("app.brokerId").setType(cp::RpcValue::Type::String).setNames("--bid", "--broker-id")
			.setDefaultValue("broker.local")
			.setComment("Broker ID string for service provider calls");
	addOption("locale").setType(cp::RpcValue::Type::String).setNames("--locale").setComment("Application locale").setDefaultValue("system");
	addOption("server.port").setType(cp::RpcValue::Type::Int).setNames("-p", "--server-port").setComment("Server TCP port").setDefaultValue(cp::IRpcConnection::DEFAULT_RPC_BROKER_PORT_NONSECURED);
	addOption("server.sslPort").setType(cp::RpcValue::Type::Int).setNames("--sslp", "--server-ssl-port").setComment("Server SSL port").setDefaultValue(cp::IRpcConnection::DEFAULT_RPC_BROKER_PORT_SECURED);
	addOption("server.discoveryPort").setType(cp::RpcValue::Type::Int).setNames("--server-discovery-ports").setComment("Server discovery UDP port").setDefaultValue(cp::IRpcConnection::DEFAULT_RPC_BROKER_PORT_NONSECURED);
#ifdef WITH_SHV_WEBSOCKETS
	addOption("server.websocket.port").setType(cp::RpcValue::Type::Int).setNames("--server-ws-port")
			.setComment("Web socket server port, set this option to enable websocket server").setDefaultValue(cp::IRpcConnection::DEFAULT_RPC_BROKER_WEB_SOCKET_PORT_NONSECURED);
	addOption("server.websocket.sslport").setType(cp::RpcValue::Type::Int).setNames("--server-wss-port")
			.setComment("Secure web socket server port, set this option to enable secure websocket server").setDefaultValue(cp::IRpcConnection::DEFAULT_RPC_BROKER_WEB_SOCKET_PORT_SECURED);
#endif
	addOption("server.ssl.key").setType(cp::RpcValue::Type::String).setNames("--server-ssl-key")
			.setComment("SSL key file").setDefaultValue("wss.key");
	addOption("server.ssl.cert").setType(cp::RpcValue::Type::String).setNames("--server-ssl-cert")
			.setComment("List of SSL certificate files").setDefaultValue("wss.crt");
	addOption("server.publicIP").setType(cp::RpcValue::Type::String).setNames("--pip", "--server-public-ip").setComment("Server public IP address");
	addOption("sqlconfig.enabled").setType(cp::RpcValue::Type::Bool).setNames("--sql-config-enabled")
			.setComment("SQL config enabled")
			.setDefaultValue(false);
	addOption("sqlconfig.driver").setType(cp::RpcValue::Type::String).setNames("--sql-config-driver")
			.setComment("SQL config database driver.")
			.setDefaultValue("QSQLITE");
	addOption("sqlconfig.database").setType(cp::RpcValue::Type::String).setNames("--sql-config-db")
			.setComment("SQL config database, if it is relative path for SQLite, {config-dir} is prepended.")
			.setDefaultValue("shvbroker.cfg.db");

	addOption("masters.connections").setType(cp::RpcValue::Type::Map).setComment("Can be used from config file only.");
	addOption("masters.enabled").setType(cp::RpcValue::Type::Bool).setNames("--mce", "--master-connections-enabled").setComment("Enable slave connections to master broker.");

#ifdef WITH_SHV_LDAP
	addOption("ldap.username").setType(cp::RpcValue::Type::String).setNames("--ldap-username").setComment("Set the LDAP username for the broker to use");
	addOption("ldap.password").setType(cp::RpcValue::Type::String).setNames("--ldap-password").setComment("Set the LDAP password for the broker to use");
	addOption("ldap.hostname").setType(cp::RpcValue::Type::String).setNames("--ldap-host").setComment("Set the LDAP server hostname");
	addOption("ldap.searchBaseDN").setType(cp::RpcValue::Type::String).setNames("--ldap-search-base-dn").setComment("Set the base DN for LDAP searches (the DN where user entries live)");
	addOption("ldap.searchAttrs").setType(cp::RpcValue::Type::List).setNames("--ldap-search-attrs").setComment("Set the LDAP attributes containing the login name for LDAP user entries");
	addOption("ldap.groupMapping").setType(cp::RpcValue::Type::List).setComment("Set the mapping of LDAP groups to shv groups as an ordered list of pairs");
#endif
	addOption("azure.groupMapping").setType(cp::RpcValue::Type::List).setComment("Set the mapping of Azure groups to shv groups as an ordered list of pairs");
}
}
