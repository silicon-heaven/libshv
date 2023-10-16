#include "remotefileloader.h"

#include <shv/coreqt/log.h>
#include <shv/core/exception.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/clientconnection.h>

#include <QCryptographicHash>
#include <QDir>

#define logRemoteFileLoader() shvCMessage("RemoteFileLoader")
#define logRemoteFileLoaderW() shvCWarning("RemoteFileLoader")

using namespace std;
using namespace shv::chainpack;

//===================================================
// RemoteFileLoader
//===================================================
RemoteFileLoader::RemoteFileLoader(shv::iotqt::rpc::ClientConnection *conn)
	: m_rpcConnection(conn)
{
	connect(this, &RemoteFileLoader::result, this, &RemoteFileLoader::deleteLater);
	connect(this, &RemoteFileLoader::error, this, &RemoteFileLoader::deleteLater);
	if(!conn)
		SHV_EXCEPTION("RPC connection is NULL");
}

RemoteFileLoader *RemoteFileLoader::create(shv::iotqt::rpc::ClientConnection *connection)
{
	return new RemoteFileLoader(connection);
}

RemoteFileLoader *RemoteFileLoader::setSiteProviderFilesPath(const QString &site_path, const QString &file_path)
{
	return setFilePath("sites/" + site_path + "/_files/" + file_path);
}

QString RemoteFileLoader::filePath() const
{
	return m_filePath;
}

RemoteFileLoader *RemoteFileLoader::setFilePath(const QString &file_path)
{
	m_filePath = file_path;
	return this;
}

/*
void RemoteFileLoader::getRemoteFile(const QString &file_path, shv::iotqt::rpc::ClientConnection *connection
							   , QObject *context, std::function<void (const QByteArray &, const QString &)> callback)
{
	RemoteFileLoader *ldr = new RemoteFileLoader(connection, file_path);
	connect(ldr, &RemoteFileLoader::dataReady, context, [callback](const QByteArray &data) {
		callback(data, QString());
	});
	connect(ldr, &RemoteFileLoader::getDataError, context, [callback](const QString &errmsg) {
		callback(QByteArray(), errmsg);
	});
	connect(ldr, &RemoteFileLoader::dataReady, ldr, &RemoteFileLoader::deleteLater);
	connect(ldr, &RemoteFileLoader::getDataError, ldr, &RemoteFileLoader::deleteLater);
	ldr->start();
}
*/
void RemoteFileLoader::start()
{
	logRemoteFileLoader() << "Starting remote file download:" << filePath();
	if(loadCacheOverlayFile())
		return;
	bool cache_hit;
	QByteArray data = loadCachedFile(cache_hit);
	if(cache_hit) {
		auto *rpc = RpcCall::create(m_rpcConnection)
						->setShvPath(filePath())
				->setMethod("hash");
		connect(rpc, &RpcCall::result, this, [this, data](const RpcValue &result) {
			const string &s = result.asString();
			QByteArray remote_hash(s.data(), s.size());
			QCryptographicHash h(QCryptographicHash::Sha1);
			h.addData(data);
			QByteArray local_hash = h.result().toHex();
			logRemoteFileLoader() << "Remote hash:" << remote_hash << "local hash:" << local_hash << "hash match:" << (local_hash == remote_hash);
			if(local_hash == remote_hash) {
				logRemoteFileLoader() << "hash match, returning cached file:" << cachedFilePath();
				emit RemoteFileLoader::result(data);
				return;
			}
			else {
				logRemoteFileLoader() << "hash not match";
				loadRemoteFile(data, true);
			}
		});
		connect(rpc, &RpcCall::error, this, [this, data](const ::shv::chainpack::RpcError &error) {
			shvWarning() << "Cannot get remote hash, error: " << error.message();
			// We cannot return local file, because cache might be populated by files from different previously visited local site
			emit RemoteFileLoader::error("Cannot get remote hash, error: " + QString::fromStdString(error.message()));
		});
		rpc->start();
	}
	else {
		logRemoteFileLoader() << "File not found in cache:" << cachedFilePath();
		loadRemoteFile(QByteArray(), false);
	}
}

QString &RemoteFileLoader::cacheDirRef()
{
	static QString s_cacheDir = QStringLiteral("%1/FL/file-cache").arg(QDir::tempPath());
	return s_cacheDir;
}

QString &RemoteFileLoader::cacheDirOverlayRef()
{
	static QString s_cacheOverlayDir;
	return s_cacheOverlayDir;
}

bool RemoteFileLoader::loadCacheOverlayFile()
{
	QString fn = cacheOverlayFilePath();
	if(!fn.isEmpty()) {
		//QDir d(s_cacheOverlayDir);
		//if(!d.exists()) {
		//	logRemoteFileLoaderW() << "Cache overlay dir not exists:" << d.absolutePath();
		//	return false;
		//}
		QFile f(fn);
		if(f.open(QFile::ReadOnly)) {
			logRemoteFileLoader() << "File found in cache overlay:" << fn;
			QByteArray data = f.readAll();
			emit result(data);
			return true;
		}
	}
	return false;
}

QByteArray RemoteFileLoader::loadCachedFile(bool &cache_hit)
{
	QString fn = cachedFilePath();
	QFile f(fn);
	if(f.open(QFile::ReadOnly)) {
		cache_hit = true;
		auto ba = f.readAll();
		logRemoteFileLoader() << "Cache hit:" << f.fileName() << "file size:" << ba.size();
		return ba;
	}
	cache_hit = false;
	return QByteArray();
}

void RemoteFileLoader::loadRemoteFile(const QByteArray fallback_data, bool fallback_data_valid)
{
	logRemoteFileLoader() << "Loading remote file:" << filePath();
	auto *rpc = RpcCall::create(m_rpcConnection)
					->setShvPath(filePath())
					->setMethod("read");
	connect(rpc, &RpcCall::result, this, [this](const RpcValue &result) {
		std::pair<const char *, size_t> data = result.asData();
		QByteArray remote_data(std::get<0>(data), std::get<1>(data));
		logRemoteFileLoader() << "Remote file downloaded, size:" << (remote_data.length() / 1024) << "kB";
		saveCachedFile(remote_data);
		emit RemoteFileLoader::result(remote_data);
	});
	connect(rpc, &RpcCall::error, this, [this, fallback_data, fallback_data_valid](const ::shv::chainpack::RpcError &err) {
		logRemoteFileLoaderW() << "Cannot get remote file:" << filePath() << "local file will be used, error:" << err.message();
		if(fallback_data_valid)
			emit result(fallback_data);
		else
			emit error(tr("File '%1' not found in cache and cannot be downloaded").arg(filePath()));
	});
	rpc->start();
}

void RemoteFileLoader::saveCachedFile(const QByteArray &data)
{
	QFileInfo fi(cachedFilePath());
	QDir d(fi.dir());
	if(!d.exists()) {
		if(!d.mkpath(d.absolutePath())) {
			logRemoteFileLoaderW() << "Cache dir not exists and cannot be created:" << d.absolutePath();
			return;
		}
		//shvWarning() << "Cache dir not exists:" << d.absolutePath();
	}
	QString fn = fi.absoluteFilePath();
	QFile f(fn);
	if(f.open(QFile::WriteOnly)) {
		logRemoteFileLoader() << "Updating cached file:" << fn << "with new content.";
		f.write(data);
	}
	else {
		logRemoteFileLoaderW() << "Cannot updateing cached file:" << fn << "with new content. File cannot be open for writing";
	}
}

QString RemoteFileLoader::cacheOverlayFilePath() const
{
	if(cacheOverlayDir().isEmpty())
		return QString();
	return cacheOverlayDir() + "/" + m_filePath;
}

QString RemoteFileLoader::cachedFilePath() const
{
	return cacheDir() + "/" + m_filePath;
}

void RemoteFileLoader::getSitesProviderFile(const QString &site_path, const QString &file_path, shv::iotqt::rpc::ClientConnection *rpc_connection, QObject *context, std::function<void (const QByteArray &, const QString &)> callback)
{
	shvMessage() << "getRemoteFile site path:" << site_path << "file path:" << file_path;
	shvInfo() << "getRemoteFile site path:" << site_path << "file path:" << file_path;
	auto *ldr = RemoteFileLoader::create(rpc_connection)
			->setSiteProviderFilesPath(site_path, file_path);
	connect(ldr, &RemoteFileLoader::result, context, [callback](const QByteArray &data) {
		callback(data, QString());
	});
	connect(ldr, &RemoteFileLoader::error, context, [site_path, file_path, callback](const QString &err_msg) {
		// direct file loading from device can cause file cache ambiguity
		// what is even worse than missing file
		QString msg  = site_path + "/" + file_path + " cannot be loaded from sites, reason: " + err_msg;
		callback(QByteArray(), msg);
#if 0
		// keep this as warning, all device _files should be cached
		shvWarning() << site_path << file_path << "cannot be loaded from sites, we will try to load from device directly, reason:" << err_msg;
		auto *ldr = RemoteFileLoader::create(rpc_connection);
		// load from device
		ldr->m_remotePath = "shv/" + site_path + "/_files/" + file_path;
		// save to sites cache
		ldr->m_filePath = "sites/" + site_path + "/_files/" + file_path;
		connect(ldr, &RemoteFileLoader::result, context, [site_path, file_path, callback](const QByteArray &data) {
			logRemoteFileLoader() << "Load file from device directly, site path:" << site_path << "files path:" << file_path << "... Ok";
			callback(data, QString());
		});
		connect(ldr, &RemoteFileLoader::error, context, [site_path, file_path, callback](const QString &err_msg) {
			logRemoteFileLoaderW() << "Load file from device directly, site path:" << site_path << "files path:" << file_path << "error:" << err_msg;
			callback(QByteArray(), err_msg);
		});
		ldr->start();
#endif
	});
	ldr->start();
}

void RemoteFileLoader::getSitesProviderFileParsed(const QString &site_path, const QString &file_path, shv::iotqt::rpc::ClientConnection *rpc_connection,
										   QObject *context, std::function<void (const RpcValue &, const QString &)> callback)
{
	getSitesProviderFile(site_path, file_path, rpc_connection, context,
									[callback](const QByteArray &data, const QString &errmsg) {
		if(errmsg.isEmpty()) {
			std::string cpon(data.constData(), data.size());
			std::string errstr;
			RpcValue rv = RpcValue::fromCpon(cpon, &errstr);
			callback(rv, QString::fromStdString(errstr));
		}
		else {
			callback(RpcValue(), "Could not read nodesTree.cpon - " + errmsg);
		}
	});
}

void RemoteFileLoader::clearCache()
{
	QString cache_dir = cacheDir();
	QDir d(cache_dir);
	d.removeRecursively();
	d.mkpath(cache_dir);
}

