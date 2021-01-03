#pragma once

#include "ConfigProps.h"

#include <cstdint>
#include <json/json.h>

class DandelionConfig
{
public:
	// Choose new Dandelion relay peer every n secs.
	uint16_t GetRelaySeconds() const { return m_relaySeconds; }

	// Dandelion embargo, fluff and broadcast tx if not seen on network before embargo expires.
	uint16_t GetEmbargoSeconds() const { return m_embargoSeconds; }

	// Dandelion patience timer, fluff/stem processing runs every n secs.
	// Tx aggregation happens on stem txs received within this window.
	uint8_t GetPatienceSeconds() const { return m_patienceSeconds; }

	// Dandelion stem probability (stem 90% of the time, fluff 10%).
	uint8_t GetStemProbability() const { return m_stemProbability; }

	//
	// Constructor
	//
	DandelionConfig(const Json::Value& json)
	{
		m_relaySeconds = 600;
		m_embargoSeconds = 180;
		m_patienceSeconds = 10;
		m_stemProbability = 90;

		if (json.isMember(ConfigProps::Dandelion::DANDELION))
		{
			const Json::Value& dandelionJSON = json[ConfigProps::Dandelion::DANDELION];

			if (dandelionJSON.isMember(ConfigProps::Dandelion::RELAY_SECS))
			{
				m_relaySeconds = (uint16_t)dandelionJSON.get(ConfigProps::Dandelion::RELAY_SECS, 600).asUInt();
			}

			if (dandelionJSON.isMember(ConfigProps::Dandelion::EMBARGO_SECS))
			{
				m_embargoSeconds = (uint16_t)dandelionJSON.get(ConfigProps::Dandelion::EMBARGO_SECS, 180).asUInt();
			}

			if (dandelionJSON.isMember(ConfigProps::Dandelion::PATIENCE_SECS))
			{
				m_patienceSeconds = (uint8_t)dandelionJSON.get(ConfigProps::Dandelion::PATIENCE_SECS, 10).asUInt();
			}

			if (dandelionJSON.isMember(ConfigProps::Dandelion::STEM_PROBABILITY))
			{
				m_stemProbability = (uint8_t)dandelionJSON.get(ConfigProps::Dandelion::STEM_PROBABILITY, 90).asUInt();
			}
		}
	}

private:
	uint16_t m_relaySeconds;
	uint16_t m_embargoSeconds;
	uint8_t m_patienceSeconds;
	uint8_t m_stemProbability;
};