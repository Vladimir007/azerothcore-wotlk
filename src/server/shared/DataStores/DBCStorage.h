#ifndef DBC_STORAGE_H
#define DBC_STORAGE_H

#include <format>
#include <map>
#include <string>

#include "DatabaseEnv.h"
#include "DBCStorageIterator.h"
#include "Errors.h"

template <class T>
class DBCStorage
{
public:
    DBCStorage() {}
    ~DBCStorage() { _data.clear(); }

    [[nodiscard]] const T* LookupEntry(const uint32 id) const
    {
        const auto res = _data.find(id);
        return res == _data.end() ? nullptr : res->second.get();
    }
    [[nodiscard]] T const* AssertEntry(const uint32 id) const
    {
        return ASSERT_NOTNULL(LookupEntry(id));
    }
    [[nodiscard]] uint32 GetNumRows() const { return _data.size(); }

    bool Load(const std::string_view table, const std::string_view fields, const std::string& ordering)
    {
        const std::string sql = std::format("SELECT {} FROM {} ORDER BY {}", fields, table, ordering);
        QueryResult result = WorldDatabase.Query(sql);
        if (!result)
            return false;
        auto item = std::make_unique<T>(result);
        _data.emplace(item->ID, std::move(item));
        return true;
    }

    DBCStorageIterator<T> begin() { return DBCStorageIterator<T>(_data.begin()); }
    DBCStorageIterator<T> end() { return DBCStorageIterator<T>(_data.end()); }

private:
    std::map<uint32, std::unique_ptr<T>> _data;

    DBCStorage(DBCStorage const& right) = delete;
    DBCStorage& operator=(DBCStorage const& right) = delete;
};

#endif
