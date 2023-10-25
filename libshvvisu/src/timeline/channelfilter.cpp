#include "channelfilter.h"

#include <shv/core/log.h>

namespace shv::visu::timeline {

ChannelFilter::ChannelFilter() = default;

ChannelFilter::ChannelFilter(const QSet<QString> &permitted_paths)
{
	m_permittedPaths = permitted_paths;
	m_isValid = true;
}

QSet<QString> ChannelFilter::permittedPaths() const
{
	return m_permittedPaths;
}

void ChannelFilter::addPermittedPath(const QString &path)
{
	m_permittedPaths.insert(path);
	m_isValid = true;
}

void ChannelFilter::removePermittedPath(const QString &path)
{
	m_permittedPaths.remove(path);
	m_isValid = true;
}

void ChannelFilter::setPermittedPaths(const QSet<QString> &paths)
{
	m_permittedPaths = paths;
	m_isValid = true;
}

bool ChannelFilter::isPathPermitted(const QString &path) const
{
	return m_permittedPaths.contains(path);
}

void ChannelFilter::setValid(bool is_valid)
{
	m_isValid = is_valid;
}

bool ChannelFilter::isValid() const
{
	return m_isValid;
}

}
