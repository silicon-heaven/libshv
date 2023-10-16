#ifndef REMOTEFILELOADER_H
#define REMOTEFILELOADER_H

#include <shv/chainpack/rpcmessage.h>
#include <shv/iotqt/rpc/rpccall.h>

#include <QObject>
#include <QVariant>

//namespace shv { namespace chainpack { class RpcValue; }}
namespace shv { namespace iotqt { namespace rpc { class ClientConnection; }}}

/*
1. gets `site_path/_files/file_path`
2. check `cache-overlay-dir` directory -> return file content if file found
3. on site provider gets remote file hash
   * if sucess
	 * check hash of cached file, update cache if needed
   * else (flatline might be offline)
	 * use cache
4. find file in cache
   * if exists -> return file content
   * if NOT exists -> return error
*/
class RemoteFileLoader : public QObject
{
	Q_OBJECT
public:
	static RemoteFileLoader* create(shv::iotqt::rpc::ClientConnection *connection);
	void start();

	Q_SIGNAL void result(const QByteArray &data);
	Q_SIGNAL void error(const QString &errmsg);

	static void getSitesProviderFile(const QString &site_path,
							  const QString &file_path,
							  shv::iotqt::rpc::ClientConnection *rpc_connection,
							  QObject *context,
							  std::function<void (const QByteArray &data, const QString &errmsg)> callback);
	static void getSitesProviderFileParsed(const QString &site_path,
							  const QString &file_path,
							  shv::iotqt::rpc::ClientConnection *rpc_connection,
							  QObject *context,
							  std::function<void (const shv::chainpack::RpcValue &data, const QString &errmsg)> callback);

	static QString cacheDir() { return cacheDirRef(); }
	static void setCacheDir(const QString &d) { cacheDirRef() = d; }
	static QString cacheOverlayDir() { return cacheDirOverlayRef(); }
	static void setCacheOverlayDir(const QString &d) { cacheDirOverlayRef() = d; }
	static void clearCache();
private:
	static QString& cacheDirRef();
	static QString& cacheDirOverlayRef();
private:
	explicit RemoteFileLoader(shv::iotqt::rpc::ClientConnection *connection);
	QString filePath() const;
	RemoteFileLoader* setFilePath(const QString &file_path);
	RemoteFileLoader* setSiteProviderFilesPath(const QString &site_path, const QString &file_path);

	QString cacheOverlayFilePath() const;
	QString cachedFilePath() const;
	bool loadCacheOverlayFile();
	void loadRemoteFile(const QByteArray fallback_data, bool fallback_data_valid);
	QByteArray loadCachedFile(bool &cache_hit);
	void saveCachedFile(const QByteArray &data);
private:
	shv::iotqt::rpc::ClientConnection *m_rpcConnection;

	QString m_filePath;
};

using RpcCall = shv::iotqt::rpc::RpcCall;

#endif // REMOTEFILELOADER_H
