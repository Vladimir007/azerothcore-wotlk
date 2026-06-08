#include "DatabaseWorker.h"
#include "PCQueue.h"
#include "SQLOperation.h"

DatabaseWorker::DatabaseWorker(ProducerConsumerQueue<SQLOperation*>* newQueue, PSQLConnection* connection)
{
    _connection = connection;
    _queue = newQueue;
    _workerThread = std::thread(&DatabaseWorker::WorkerThread, this);
}

DatabaseWorker::~DatabaseWorker()
{
    _workerThread.join();
}

void DatabaseWorker::WorkerThread()
{
    if (!_queue)
        return;

    for (;;)
    {
        SQLOperation* operation = nullptr;

        _queue->WaitAndPop(operation);

        if (!operation)
            return;

        operation->SetConnection(_connection);
        operation->call();

        delete operation;
    }
}
