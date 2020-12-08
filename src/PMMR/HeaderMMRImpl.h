#pragma once

#include "Common/HashFile.h"

#include <PMMR/HeaderMMR.h>
#include <Core/Models/BlockHeader.h>
#include <optional>
#include <string>

class HeaderMMR : public IHeaderMMR
{
public:
	static std::shared_ptr<HeaderMMR> Load(const fs::path& path);

	void AddHeader(const BlockHeader& header) final;
	Hash Root(const uint64_t lastHeight) const final;
	void Rewind(const uint64_t size) final;

	void Commit() final;
	void Rollback() noexcept final;

private:
	HeaderMMR(std::shared_ptr<Locked<HashFile>> pHashFile);

	std::shared_ptr<Locked<HashFile>> m_pLockedHashFile;

	void OnInitWrite(const bool /*batch*/) final
	{
		SetDirty(false);

		BatchData batch;
		batch.hashFile = m_pLockedHashFile->BatchWrite();
		m_batchDataOpt = std::make_optional(std::move(batch));
	}

	void OnEndWrite() final
	{
		m_batchDataOpt = std::nullopt;
	}

	struct BatchData
	{
		Writer<HashFile> hashFile;
	};
	std::optional<BatchData> m_batchDataOpt;
};
