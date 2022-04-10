#pragma once
#include "sqlite3.h"
#include<string>

int OpenDatabase(const char* database_name, sqlite3* & db);

int DropTable(sqlite3 * db, const char* sql_stmt);

int ExecuteSQL(sqlite3* db, const char* sql_stmt);

int ShowTable(sqlite3* db, const char* sql_stmt);

int GetTable(sqlite3* db, const char* sql_stmt, char*** result_ptr, int* rows_ptr, int* columns_ptr);

void CloseDatabase(sqlite3* db);

bool IsTableExist(sqlite3* db, const std::string& table_name);
