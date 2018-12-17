#pragma once

#include <stdint.h>

class DandelionConfig
{
public:
	DandelionConfig(const uint16_t relaySeconds, const uint16_t embargoSeconds, const uint8_t patienceSeconds, const uint8_t stemProbability)
		: m_relaySeconds(relaySeconds), m_embargoSeconds(embargoSeconds), m_patienceSeconds(patienceSeconds), m_stemProbability(stemProbability)
	{

	}

	// Choose new Dandelion relay peer every n secs.
	inline const uint16_t GetRelaySeconds() const { return m_relaySeconds; }

	// Dandelion embargo, fluff and broadcast tx if not seen on network before embargo expires.
	inline const uint16_t GetEmbargoSeconds() const { return m_embargoSeconds; }

	// Dandelion patience timer, fluff/stem processing runs every n secs.
	// Tx aggregation happens on stem txs received within this window.
	inline const uint8_t GetPatienceSeconds() const { return m_patienceSeconds; }

	// Dandelion stem probability (stem 90% of the time, fluff 10%).
	inline const uint8_t GetStemProbability() const { return m_stemProbability; }

private:
	uint16_t m_relaySeconds;
	uint16_t m_embargoSeconds;
	uint8_t m_patienceSeconds;
	uint8_t m_stemProbability;
};