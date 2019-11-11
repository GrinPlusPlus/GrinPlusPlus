#pragma once

#include "Common/HashFile.h"

#include <PMMR/HeaderMMR.h>
#include <Core/Models/BlockHeader.h>
#include <optional>
#include <string>

class HeaderMMR : public IHeaderMMR
{
public:
	static std::shared_ptr<HeaderMMR> Load(const std::string& path);

	virtual void AddHeader(const BlockHeader& header) override final;
	virtual Hash Root(const uint64_t lastHeight) const override final;
	virtual void Rewind(const uint64_t size) override final;

	virtual void Commit() override final;
	virtual void Rollback() override final;

private:
	HeaderMMR(std::shared_ptr<Locked<HashFile>> pHashFile);

	std::shared_ptr<Locked<HashFile>> m_pLockedHashFile;

	virtual void OnInitWrite() override final
	{
		BatchData batch;
		batch.hashFile = m_pLockedHashFile->BatchWrite();
		m_batchDataOpt = std::make_optional(std::move(batch));
	}

	virtual void OnEndWrite() override final
	{
		m_batchDataOpt = std::nullopt;
	}

	struct BatchData
	{
		Writer<HashFile> hashFile;
	};
	std::optional<BatchData> m_batchDataOpt;
};
