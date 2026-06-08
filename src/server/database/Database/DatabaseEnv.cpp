#include "DatabaseEnv.h"

DatabaseWorkerPool<WorldDatabaseConnection> WorldDatabase;
DatabaseWorkerPool<CharacterDatabaseConnection> CharacterDatabase;
DatabaseWorkerPool<LoginDatabaseConnection> LoginDatabase;
