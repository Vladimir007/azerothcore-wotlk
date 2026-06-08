#ifndef DATABASE_ENV_H
#define DATABASE_ENV_H

#include "DatabaseWorkerPool.h"

#include "Implementation/CharacterDatabase.h"
#include "Implementation/LoginDatabase.h"
#include "Implementation/WorldDatabase.h"

#include "PreparedStatement.h"
#include "QueryCallback.h"
#include "QueryResult.h"
#include "Transaction.h"
#include "Field.h"

/// Accessor to the world database
extern DatabaseWorkerPool<WorldDatabaseConnection> WorldDatabase;
/// Accessor to the character database
extern DatabaseWorkerPool<CharacterDatabaseConnection> CharacterDatabase;
/// Accessor to the realm/login database
extern DatabaseWorkerPool<LoginDatabaseConnection> LoginDatabase;

#endif
