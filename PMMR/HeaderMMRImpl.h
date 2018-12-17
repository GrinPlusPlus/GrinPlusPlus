#pragma once

#include "Common/HashFile.h"

#include <HeaderMMR.h>
#include <Core/BlockHeader.h>
#include <string>

class HeaderMMR : public IHeaderMMR
{
public:
	HeaderMMR(const std::string& path);

	bool Load();
	virtual bool Commit() override final;
	virtual bool Rewind(const uint64_t size) override final;
	virtual bool Rollback() override final;

	virtual void AddHeader(const BlockHeader& header) override final;
	virtual Hash Root(const uint64_t lastHeight) const override final;

private:
	Hash HashWithIndex(const BlockHeader& header, const uint64_t index) const;

	HashFile m_hashFile;
	std::vector<Hash> m_hashes;
};