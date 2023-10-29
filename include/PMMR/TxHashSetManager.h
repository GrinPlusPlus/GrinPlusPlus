#pragma once

#include <PMMR/TxHashSet.h>
#include <Core/Config.h>
#include <Core/Traits/Lockable.h>
#include <filesystem.h>
#include <memory>

// Forward Declarations
class IBlockDB;

class TxHashSetManager : public Traits::IBatchable
{
public:
	using Ptr = std::shared_ptr<TxHashSetManager>;
	using CPtr = std::shared_ptr<TxHashSetManager>;

	TxHashSetManager(const Config& config);
	~TxHashSetManager() = default;

	std::shared_ptr<ITxHashSet> Open(const BlockHeaderPtr& pConfirmedTip);
	void Close()
	{ 
		if(m_pTxHashSet != nullptr)
		{
			m_pTxHashSet = std::shared_ptr<ITxHashSet>(nullptr);
		}
		delete m_pTxHashSet.get();
	}

	std::shared_ptr<ITxHashSet> GetTxHashSet() { return m_pTxHashSet; }
	std::shared_ptr<const ITxHashSet> GetTxHashSet() const { return m_pTxHashSet; }
	void SetTxHashSet(ITxHashSetPtr pTxHashSet) { m_pTxHashSet = pTxHashSet; }

	static ITxHashSetPtr LoadFromZip(const Config& config, const fs::path& zipFilePath, BlockHeaderPtr pHeader);
	fs::path SaveSnapshot(std::shared_ptr<IBlockDB> pBlockDB, BlockHeaderPtr pHeader) const;

	void Commit() final
	{
		if (m_pTxHashSet != nullptr)
		{
			m_pTxHashSet->Commit();
		}
	}

	void Rollback() noexcept final
	{
		if (m_pTxHashSet != nullptr)
		{
			m_pTxHashSet->Rollback();
		}
	}

private:
	const Config& m_config;
	std::shared_ptr<ITxHashSet> m_pTxHashSet;
};