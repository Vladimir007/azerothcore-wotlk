#ifndef ADHOC_STATEMENT_H
#define ADHOC_STATEMENT_H

#include "DatabaseEnvFwd.h"
#include "SQLOperation.h"

class BasicStatementTask : public SQLOperation
{
public:
    explicit BasicStatementTask(std::string_view sql, bool async = false);
    ~BasicStatementTask() override;

    bool Execute() override;
    QueryResultFuture GetFuture() const { return m_result->get_future(); }

private:
    std::string m_sql; //- Raw query to be executed
    bool m_has_result;
    QueryResultPromise* m_result;
};

#endif
