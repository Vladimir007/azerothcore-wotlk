#ifndef QUERY_RESULT_H
#define QUERY_RESULT_H

#include <pqxx/pqxx>
#include "DatabaseEnvFwd.h"
#include "Define.h"


template<typename T>
struct ResultIterator
{
    explicit ResultIterator(T* ptr) : _ptr(ptr) { }

    T& operator*() const { return *_ptr; }
    T* operator->() { return _ptr; }
    ResultIterator& operator++() { if (!_ptr->NextRow()) _ptr = nullptr; return *this; }

    bool operator!=(const ResultIterator& right) { return _ptr != right._ptr; }

private:
    T* _ptr;
};

class ResultSet
{
public:
    explicit ResultSet(const pqxx::result& result);
    ~ResultSet();

    bool NextRow();
    [[nodiscard]] uint64 GetRowCount() const { return _rowCount; }
    [[nodiscard]] uint32 GetFieldCount() const { return _fieldCount; }

    [[nodiscard]] Field* Fetch() const;
    Field const& operator[](std::size_t index) const;

    auto begin()      { return ResultIterator(this); }
    static auto end() { return ResultIterator<ResultSet>(nullptr); }

protected:
    uint64 _rowCount;
    Field* _currentRow;
    uint32 _fieldCount;

private:
    void CleanUp();

    uint64 _currentRowInd{0};
    pqxx::result _result;
};
#endif
