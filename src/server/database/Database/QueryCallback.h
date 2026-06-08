#ifndef QUERY_CALLBACK_H
#define QUERY_CALLBACK_H

#include <functional>
#include <future>
#include <list>
#include <queue>
#include "DatabaseEnvFwd.h"

class QueryCallback
{
public:
    explicit QueryCallback(QueryResultFuture&& result, bool prepared);

    QueryCallback(QueryCallback&& right) noexcept;
    QueryCallback& operator=(QueryCallback&& right) noexcept;
    ~QueryCallback();

    QueryCallback&& WithCallback(std::function<void(QueryResult)>&& callback);
    QueryCallback&& WithPreparedCallback(std::function<void(QueryResult)>&& callback);

    QueryCallback&& WithChainingCallback(std::function<void(QueryCallback&, QueryResult)>&& callback);
    QueryCallback&& WithChainingPreparedCallback(std::function<void(QueryCallback&, QueryResult)>&& callback);

    // Moves std::future from next to this object
    void SetNextQuery(QueryCallback&& next);

    // returns true when completed
    bool InvokeIfReady();

private:
    QueryCallback(QueryCallback const& right) = delete;
    QueryCallback& operator=(QueryCallback const& right) = delete;

    template<typename T> friend void ConstructActiveMember(T* obj);
    template<typename T> friend void DestroyActiveMember(T* obj);
    template<typename T> friend void MoveFrom(T* to, T&& from);

    union
    {
        QueryResultFuture _string;
        QueryResultFuture _prepared;
    };

    bool _isPrepared;

    struct QueryCallbackData;
    std::queue<QueryCallbackData, std::list<QueryCallbackData>> _callbacks;
};

#endif
