#pragma once

#include "Common/HashFile.h"

#include <PMMR/HeaderMMR.h>
#include <Core/Models/BlockHeader.h>
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
	HashFile m_hashFile;
};