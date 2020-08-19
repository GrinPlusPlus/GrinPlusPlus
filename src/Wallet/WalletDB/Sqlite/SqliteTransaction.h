#pragma once

#include "SqliteDB.h"

#include <Common/Util/StringUtil.h>
#include <Wallet/WalletDB/WalletStoreException.h>
#include <Common/Logger.h>

class SqliteTransaction
{
public:
	SqliteTransaction(const SqliteDB::Ptr& pDatabase)
		: m_pDatabase(pDatabase), m_open(false) { }

	~SqliteTransaction()
	{
		if (m_open) {
			Rollback();
		}
	}

	void Begin()
	{
		if (m_open) {
			WALLET_ERROR("Tried to begin a transaction twice");
			throw WALLET_STORE_EXCEPTION("Tried to begin a transaction twice.");
		}

		m_pDatabase->Execute("BEGIN;");

		m_open = true;
	}

	void Commit()
	{
		if (!m_open) {
			WALLET_ERROR("Tried to commit a closed transaction");
			throw WALLET_STORE_EXCEPTION("Tried to commit a closed transaction.");
		}

		m_open = false;

		m_pDatabase->Execute("COMMIT;");
	}

	void Rollback() noexcept
	{
		if (!m_open) {
			WALLET_ERROR("Tried to rollback a closed transaction");
			return;
		}

		m_open = false;

		m_pDatabase->Execute("ROLLBACK;");
	}

private:
	SqliteDB::Ptr m_pDatabase;
	bool m_open;
};