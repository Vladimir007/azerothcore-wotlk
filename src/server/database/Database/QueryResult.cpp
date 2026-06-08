#include "QueryResult.h"

#include "Errors.h"
#include "Field.h"


ResultSet::ResultSet(const pqxx::result& result): _result(result)
{

    _rowCount = std::size(_result);
    _fieldCount = _result.columns();
    _currentRow = new Field[_fieldCount];
}

ResultSet::~ResultSet()
{
    CleanUp();
}

Field* ResultSet::Fetch() const
{
    const auto row = _result[_currentRowInd];
    for (uint32 i = 0; i < _fieldCount; ++i)
    {
        _currentRow[i].Set(row[i]);
    }
    return _currentRow;
}

bool ResultSet::NextRow()
{
    if (++_currentRowInd >= _rowCount)
        return false;
    return true;
}

void ResultSet::CleanUp()
{
    if (_currentRow)
    {
        delete[] _currentRow;
        _currentRow = nullptr;
    }
    _result.clear();
}

Field const& ResultSet::operator[](const std::size_t index) const
{
    ASSERT(index < _fieldCount);
    return _currentRow[index];
}
