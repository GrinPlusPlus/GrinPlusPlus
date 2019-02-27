#pragma once

#include <Core/Models/Transaction.h>
#include <Wallet/ParticipantData.h>
#include <uuid.h>

#include <stdint.h>

// A 'Slate' is passed around to all parties to build up all of the public
// transaction data needed to create a finalized transaction. Callers can pass
// the slate around by whatever means they choose, (but we can provide some
// binary or JSON serialization helpers here).
class Slate
{
public:
	Slate(const uint64_t numParticipants, uuids::uuid&& slateId, Transaction&& transaction, const uint64_t amount, const uint64_t fee, const uint64_t blockHeight, const uint64_t lockHeight)
		: m_numParticipants(numParticipants),
		m_slateId(std::move(slateId)),
		m_transaction(std::move(transaction)),
		m_amount(amount),
		m_fee(fee),
		m_blockHeight(blockHeight),
		m_lockHeight(lockHeight)
	{

	}

	inline const uuids::uuid& GetSlateId() const { return m_slateId; }
	inline uint64_t GetAmount() const { return m_amount; }
	inline uint64_t GetFee() const { return m_fee; }
	inline const Transaction& GetTransaction() const { return m_transaction; }
	inline const std::vector<ParticipantData>& GetParticipantData() const { return m_participantData; }

	inline void AddParticpantData(const ParticipantData& participantData) { m_participantData.push_back(participantData); }

private:
	// The number of participants intended to take part in this transaction
	uint64_t m_numParticipants;
	// Unique transaction/slate ID, selected by sender
	uuids::uuid m_slateId;
	// The core transaction data:
	// inputs, outputs, kernels, kernel offset
	Transaction m_transaction;
	// base amount (excluding fee)
	uint64_t m_amount;
	// fee amount
	uint64_t m_fee;
	// Block height for the transaction
	uint64_t m_blockHeight;
	// Lock height
	uint64_t m_lockHeight;
	// Participant data, each participant in the transaction will insert their public data here.
	// For now, 0 is sender and 1 is receiver, though this will change for multi-party.
	std::vector<ParticipantData> m_participantData;
};