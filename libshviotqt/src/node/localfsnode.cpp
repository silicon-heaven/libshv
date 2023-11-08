#include "localfsnode.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/exception.h>
#include <shv/core/stringview.h>
#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace node {

static const char M_WRITE[] = "write";
static const char M_DELETE[] = "delete";
static const char M_MKFILE[] = "mkfile";
static const char M_MKDIR[] = "mkdir";
static const char M_RMDIR[] = "rmdir";

static const std::vector<cp::MetaMethod> meta_methods_dir {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{M_MKFILE, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_WRITE},
	{M_MKDIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_WRITE},
	{M_RMDIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_SERVICE}
};

static const std::vector<cp::MetaMethod> meta_methods_dir_write_file {
	{M_WRITE, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_WRITE},
};

static const std::vector<cp::MetaMethod> meta_methods_file_modificable {
	{M_WRITE, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_WRITE},
	{M_DELETE, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::ROLE_SERVICE}
};

static const std::vector<cp::MetaMethod> & get_meta_methods_file()
{
	static std::vector<cp::MetaMethod> meta_methods;
	if (meta_methods.empty()) {
		meta_methods = FileNode::meta_methods_file_base;
		meta_methods.insert(meta_methods.end(), meta_methods_file_modificable.begin(), meta_methods_file_modificable.end());
	}
	return meta_methods;

}

LocalFSNode::LocalFSNode(const QString &root_path, ShvNode *parent)
	: Super({}, parent)
	, m_rootDir(root_path)
{
}

LocalFSNode::LocalFSNode(const QString &root_path, const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
	, m_rootDir(root_path)
{
}

chainpack::RpcValue LocalFSNode::callMethod(const ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(method == M_WRITE) {
		return ndWrite(QString::fromStdString(shv_path.join('/')), params);
	}
	else if(method == M_DELETE) {
		return ndDelete(QString::fromStdString(shv_path.join('/')));
	}
	else if(method == M_MKFILE) {
		return ndMkfile(QString::fromStdString(shv_path.join('/')), params);
	}
	else if(method == M_MKDIR) {
		return ndMkdir(QString::fromStdString(shv_path.join('/')), params);
	}
	else if(method == M_RMDIR) {
		bool recursively = (params.isBool()) ? params.toBool() : false;
		return ndRmdir(QString::fromStdString(shv_path.join('/')), recursively);
	}

	return Super::callMethod(shv_path, method, params, user_id);
}

ShvNode::StringList LocalFSNode::childNames(const ShvNode::StringViewList &shv_path)
{
	QString qpath = QString::fromStdString(shv_path.join('/'));
	QFileInfo fi_path(makeAbsolutePath(qpath));
	//shvInfo() << __FUNCTION__ << fi_path.absoluteFilePath() << "is dir:" << fi_path.isDir();
	if(fi_path.isDir()) {
		QDir d2(fi_path.absoluteFilePath());
		if(!d2.exists())
			SHV_EXCEPTION("Path " + d2.absolutePath().toStdString() + " do not exists.");
		ShvNode::StringList lst;
		for(const QFileInfo &fi : d2.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase | QDir::DirsFirst)) {
			//shvInfo() << fi.fileName();
			lst.push_back(fi.fileName().toStdString());
		}
		return lst;
	}
	return ShvNode::StringList();
}

chainpack::RpcValue LocalFSNode::hasChildren(const ShvNode::StringViewList &shv_path)
{
	return isDir(shv_path);
}

size_t LocalFSNode::methodCount(const ShvNode::StringViewList &shv_path)
{
	QFileInfo fi = ndFileInfo(QString::fromStdString(shv_path.join('/')));
	if(fi.exists())
		return isDir(shv_path) ? meta_methods_dir.size() : get_meta_methods_file().size();
	if(!shv_path.empty()) {
		StringViewList dir_path = shv_path.mid(0, shv_path.size() - 1);
		fi = ndFileInfo(QString::fromStdString(dir_path.join('/')));
		if(fi.isDir())
			return meta_methods_dir_write_file.size();
	}
	return 0;
}

const chainpack::MetaMethod *LocalFSNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	QFileInfo fi = ndFileInfo(QString::fromStdString(shv_path.join('/')));
	if(fi.exists()) {
		size_t meta_methods_size = (fi.isDir()) ? meta_methods_dir.size() : get_meta_methods_file().size();

		if(meta_methods_size <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_size));

		return (fi.isDir()) ? &(meta_methods_dir[ix]) : &(get_meta_methods_file()[ix]);
	}
	if(!shv_path.empty()) {
		StringViewList dir_path = shv_path.mid(0, shv_path.size() - 1);
		fi = ndFileInfo(QString::fromStdString(dir_path.join('/')));
		if(fi.isDir() && ix < meta_methods_dir_write_file.size())
			return &(meta_methods_dir_write_file[ix]);
	}
	SHV_EXCEPTION("Invalid method index: " + std::to_string(ix));
}

QString LocalFSNode::makeAbsolutePath(const QString &relative_path) const
{
	return m_rootDir.absolutePath() + '/' + relative_path;
}

std::string LocalFSNode::fileName(const ShvNode::StringViewList &shv_path) const
{
	return shv_path.size() > 0 ? shv_path.back().toString() : "";
}

chainpack::RpcValue LocalFSNode::readContent(const ShvNode::StringViewList &shv_path) const
{
	return ndRead(QString::fromStdString(shv_path.join('/')));
}

chainpack::RpcValue LocalFSNode::size(const ShvNode::StringViewList &shv_path) const
{
	return ndSize(QString::fromStdString(shv_path.join('/')));
}

bool LocalFSNode::isDir(const ShvNode::StringViewList &shv_path) const
{
	QString qpath = QString::fromStdString(shv_path.join('/'));
	QFileInfo fi(makeAbsolutePath(qpath));
	if(!fi.exists())
		shvError() << "Invalid path:" << fi.absoluteFilePath();
	shvDebug() << __FUNCTION__ << "shv path:" << qpath << "file info:" << fi.absoluteFilePath() << "is dir:" << fi.isDir();
	return fi.isDir();
}

void LocalFSNode::checkPathIsBoundedToFsRoot(const QString &path) const
{
	if (!QDir::cleanPath(path).startsWith(m_rootDir.absolutePath())) {
		SHV_EXCEPTION("Path:" + path.toStdString() + " is out of fs root directory:" + m_rootDir.path().toStdString());
	}
}

QFileInfo LocalFSNode::ndFileInfo(const QString &path) const
{
	QFileInfo fi(makeAbsolutePath(path));
	return fi;
}

cp::RpcValue LocalFSNode::ndSize(const QString &path) const
{
	return (unsigned)ndFileInfo(path).size();
}

chainpack::RpcValue LocalFSNode::ndRead(const QString &path) const
{
	QString file_path = makeAbsolutePath(path);
	checkPathIsBoundedToFsRoot(file_path);

	QFile f(file_path);
	if(f.open(QFile::ReadOnly)) {
		QByteArray ba = f.readAll();
		return cp::RpcValue::Blob(ba.constData(), ba.constData() + ba.size());
	}
	SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for reading.");
}

chainpack::RpcValue LocalFSNode::ndWrite(const QString &path, const chainpack::RpcValue &methods_params)
{
	QFile f(makeAbsolutePath(path));

	if (methods_params.isString()){
		if(f.open(QFile::WriteOnly)) {
			const chainpack::RpcValue::String &content = methods_params.asString();
			f.write(content.data(), content.size());
			return true;
		}
		SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for writing.");
	}
	else if (methods_params.isList()){
		chainpack::RpcValue::List params = methods_params.toList();

		if (params.size() != 2){
			SHV_EXCEPTION("Cannot write to file " + f.fileName().toStdString() + ". Invalid parameters count.");
		}
		chainpack::RpcValue::Map flags = params[1].asMap();
		QFile::OpenMode open_mode = (flags.value("append").toBool()) ? QFile::Append : QFile::WriteOnly;

		if(f.open(open_mode)) {
			const chainpack::RpcValue::String &content = params[0].toString();
			f.write(content.data(), content.size());
			return true;
		}
		SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for writing.");
	}
	else{
		SHV_EXCEPTION("Unsupported param type.");
	}

	return false;
}

chainpack::RpcValue LocalFSNode::ndDelete(const QString &path)
{
	QString file_path = makeAbsolutePath(path);
	checkPathIsBoundedToFsRoot(file_path);

	QFile file (file_path);
	return file.remove();
}

chainpack::RpcValue LocalFSNode::ndMkfile(const QString &path, const chainpack::RpcValue &methods_params)
{
	std::string error;

	if (methods_params.isString()){
		QString file_path = makeAbsolutePath(path + '/' + QString::fromStdString(methods_params.asString()));
		checkPathIsBoundedToFsRoot(file_path);

		QFile f(file_path);
		if(f.open(QFile::WriteOnly)) {
			return true;
		}
		error = "Cannot open file " + file_path.toStdString() + " for writing.";
	}
	else if (methods_params.isList()){
		const chainpack::RpcValue::List &param_lst = methods_params.toList();

		if (param_lst.size() != 2) {
			throw shv::core::Exception("Invalid params, [\"name\", \"content\"] expected.");
		}

		QString file_path = makeAbsolutePath(path + '/' + QString::fromStdString(param_lst[0].asString()));
		QDir d(QFileInfo(file_path).dir());

		if (!d.mkpath(d.absolutePath())){
			SHV_EXCEPTION("Cannot create path " + file_path.toStdString() + ".");
		}

		QFile f(file_path);

		if (f.exists()){
			SHV_EXCEPTION("File " + f.fileName().toStdString() + " already exists.");
		}

		if(f.open(QFile::WriteOnly)) {
			const std::string &data = param_lst[1].asString();
			f.write(data.data(), data.size());
			return true;
		}
		SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for writing.");
	}
	else{
		SHV_EXCEPTION("Unsupported param type.");
	}

	return false;
}

chainpack::RpcValue LocalFSNode::ndMkdir(const QString &path, const chainpack::RpcValue &methods_params)
{
	QString dir_path = makeAbsolutePath(path);
	checkPathIsBoundedToFsRoot(dir_path);

	QDir d(dir_path);

	if (!methods_params.isString())
		SHV_EXCEPTION("Cannot create directory in directory " + d.absolutePath().toStdString() + ". Invalid parameter: " + methods_params.toCpon());

	return d.mkpath(makeAbsolutePath(path + '/' + QString::fromStdString(methods_params.asString())));
}

chainpack::RpcValue LocalFSNode::ndRmdir(const QString &path, bool recursively)
{
	QString file_path = makeAbsolutePath(path);
	checkPathIsBoundedToFsRoot(file_path);

	QDir d(file_path);

	if (path.isEmpty())
		SHV_EXCEPTION("Cannot remove root directory " + d.absolutePath().toStdString());

	if (recursively)
		return d.removeRecursively();
	else
		return d.rmdir(makeAbsolutePath(path));
}

} // namespace node
} // namespace iotqt
} // namespace shv
