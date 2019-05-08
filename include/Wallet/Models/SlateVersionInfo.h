#pragma once

#include <Core/Util/JsonUtil.h>
#include <json/json.h>
#include <stdint.h>

class SlateVersionInfo
{
public:
	SlateVersionInfo(const uint16_t version)
		: m_version(version), m_originalVersion(version), m_minCompatVersion(0)
	{

	}

	SlateVersionInfo(const uint16_t version, const uint16_t originalVersion, const uint16_t minCompatVersion)
		: m_version(version), m_originalVersion(originalVersion), m_minCompatVersion(minCompatVersion)
	{

	}

	inline const uint16_t GetVersion() const { return m_version; }
	inline const uint16_t GetOriginalVersion() const { return m_originalVersion; }
	inline const uint16_t GetMinimumCompatibleVersion() const { return m_minCompatVersion; }

	Json::Value ToJSON() const
	{
		Json::Value versionJSON;
		versionJSON["version"] = m_version;
		versionJSON["orig_version"] = m_originalVersion;
		versionJSON["min_compat_version"] = m_minCompatVersion;
		return versionJSON;
	}

	static SlateVersionInfo FromJSON(const Json::Value& versionInfoJSON)
	{
		const uint16_t version = (uint16_t)JsonUtil::GetRequiredField(versionInfoJSON, "version").asUInt();
		const uint16_t origVersion = (uint16_t)JsonUtil::GetRequiredField(versionInfoJSON, "orig_version").asUInt();
		const uint16_t minCompatVersion = (uint16_t)JsonUtil::GetRequiredField(versionInfoJSON, "min_compat_version").asUInt();
		return SlateVersionInfo(version, origVersion, minCompatVersion);
	}

private:
	uint16_t m_version;
	uint16_t m_originalVersion;
	uint16_t m_minCompatVersion;
};