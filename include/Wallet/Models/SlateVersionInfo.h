#pragma once

#include <Core/Util/JsonUtil.h>
#include <json/json.h>
#include <stdint.h>

class SlateVersionInfo
{
public:
	SlateVersionInfo(const uint16_t version)
		: m_version(version), m_originalVersion(version), m_blockHeaderVersion(1)
	{

	}

	SlateVersionInfo(const uint16_t version, const uint16_t originalVersion, const uint16_t blockHeaderVersion)
		: m_version(version), m_originalVersion(originalVersion), m_blockHeaderVersion(blockHeaderVersion)
	{

	}

	inline const uint16_t GetVersion() const { return m_version; }
	inline const uint16_t GetOriginalVersion() const { return m_originalVersion; }
	inline const uint16_t GetBlockHeaderVersion() const { return m_blockHeaderVersion; }

	Json::Value ToJSON() const
	{
		Json::Value versionJSON;
		versionJSON["version"] = m_version;
		versionJSON["orig_version"] = m_originalVersion;
		versionJSON["block_header_version"] = m_blockHeaderVersion;
		return versionJSON;
	}

	static SlateVersionInfo FromJSON(const Json::Value& versionInfoJSON)
	{
		const uint16_t version = (uint16_t)JsonUtil::GetRequiredField(versionInfoJSON, "version").asUInt();
		const uint16_t origVersion = (uint16_t)JsonUtil::GetRequiredField(versionInfoJSON, "orig_version").asUInt();
		const uint16_t blockHeaderVersion = (uint16_t)JsonUtil::GetRequiredField(versionInfoJSON, "block_header_version").asUInt();
		return SlateVersionInfo(version, origVersion, blockHeaderVersion);
	}

private:
	uint16_t m_version;
	uint16_t m_originalVersion;
	uint16_t m_blockHeaderVersion;
};