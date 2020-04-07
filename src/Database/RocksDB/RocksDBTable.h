#pragma once

#include <Core/Traits/Printable.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>

class RocksDBTable : public Traits::IPrintable
{
public:
    RocksDBTable() = default;
    RocksDBTable(const std::string & name, rocksdb::ColumnFamilyHandle* pFamilyHandle)
        : m_name(name), m_pFamilyHandle(std::shared_ptr<rocksdb::ColumnFamilyHandle>(pFamilyHandle)) { }
    RocksDBTable(const std::string& name, const std::shared_ptr<rocksdb::ColumnFamilyHandle>& pFamilyHandle)
        : m_name(name), m_pFamilyHandle(pFamilyHandle) { }
    virtual ~RocksDBTable() = default;

    const std::string& GetName() const noexcept { return m_name; }
    rocksdb::ColumnFamilyHandle* GetHandle() const { return m_pFamilyHandle.get(); }

    void CloseHandle() { m_pFamilyHandle.reset(); }

    std::string Format() const final { return m_name; }

private:
    std::string m_name;
    std::shared_ptr<rocksdb::ColumnFamilyHandle> m_pFamilyHandle;
};