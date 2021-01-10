#pragma once

#include <filesystem.h>
#include <string>
#include <vector>
#include <memory>

// Forward Declarations
struct sqlite3;
struct sqlite3_stmt;

class SqliteDB
{
public:
    class Statement
    {
    public:
        using UPtr = std::unique_ptr<Statement>;

        Statement(sqlite3* pDatabase, sqlite3_stmt* pStatement)
            : m_pDatabase(pDatabase), m_pStatement(pStatement), m_finalized(false) { }
        ~Statement();

        bool Step();
        void Finalize();

        bool IsColumnNull(const int col) const;
        int GetColumnInt(const int col) const;
        int64_t GetColumnInt64(const int col) const;
        std::vector<uint8_t> GetColumnBytes(const int col) const;
        std::string GetColumnString(const int col) const;

    private:
        sqlite3* m_pDatabase;
        sqlite3_stmt* m_pStatement;
        bool m_finalized;
    };

    class IParameter
    {
    public:
        using UPtr = std::unique_ptr<IParameter>;
        virtual ~IParameter() = default;

        virtual void Bind(sqlite3_stmt* stmt, const int index) const = 0;
    };

    using Ptr = std::shared_ptr<SqliteDB>;

    static SqliteDB::Ptr Open(const fs::path& db_path, const std::string& username);

    SqliteDB(sqlite3* pDatabase, const std::string& username)
        : m_pDatabase(pDatabase), m_username(username) { }
    ~SqliteDB();

    void Execute(const std::string& command);
    void Update(const std::string& statement, const std::vector<IParameter::UPtr>& parameters = {});
    Statement::UPtr Query(const std::string& query, const std::vector<IParameter::UPtr>& parameters = {});
    std::string GetError() const;

private:
    sqlite3* m_pDatabase;
    std::string m_username;
};

class TextParameter : public SqliteDB::IParameter
{
public:
    TextParameter(const std::string& value) : m_value(value) { }
    static SqliteDB::IParameter::UPtr New(const std::string& value)
    {
        return SqliteDB::IParameter::UPtr((SqliteDB::IParameter*)new TextParameter(value));
    }

    void Bind(sqlite3_stmt* stmt, const int index) const final;

private:
    std::string m_value;
};

class IntParameter : public SqliteDB::IParameter
{
public:
    IntParameter(const int value) : m_value(value) { }
    static SqliteDB::IParameter::UPtr New(const int value)
    {
        return SqliteDB::IParameter::UPtr((SqliteDB::IParameter*)new IntParameter(value));
    }

    void Bind(sqlite3_stmt* stmt, const int index) const final;

private:
    int m_value;
};

class BlobParameter : public SqliteDB::IParameter
{
public:
    BlobParameter(const std::vector<uint8_t>& blob) : m_blob(blob) { }
    static SqliteDB::IParameter::UPtr New(const std::vector<uint8_t>& blob)
    {
        return SqliteDB::IParameter::UPtr((SqliteDB::IParameter*)new BlobParameter(blob));
    }

    void Bind(sqlite3_stmt* stmt, const int index) const final;

private:
    std::vector<uint8_t> m_blob;
};

class NullParameter : public SqliteDB::IParameter
{
public:
    NullParameter() = default;
    static SqliteDB::IParameter::UPtr New()
    {
        return SqliteDB::IParameter::UPtr((SqliteDB::IParameter*)new NullParameter());
    }

    void Bind(sqlite3_stmt* stmt, const int index) const final;
};