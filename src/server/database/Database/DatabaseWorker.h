#ifndef DATABASE_WORKER_H
#define DATABASE_WORKER_H

#include <thread>

template <typename T>
class ProducerConsumerQueue;

class PSQLConnection;
class SQLOperation;

class DatabaseWorker
{
public:
    DatabaseWorker(ProducerConsumerQueue<SQLOperation*>* newQueue, PSQLConnection* connection);
    ~DatabaseWorker();

private:
    ProducerConsumerQueue<SQLOperation*>* _queue;
    PSQLConnection* _connection;

    void WorkerThread();
    std::thread _workerThread;

    DatabaseWorker(DatabaseWorker const& right) = delete;
    DatabaseWorker& operator=(DatabaseWorker const& right) = delete;
};

#endif
