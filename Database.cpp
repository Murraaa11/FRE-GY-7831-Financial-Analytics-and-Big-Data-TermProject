#include "Database.h"
#include <iostream>

int OpenDatabase(const char* database_name, sqlite3* & db)
{
	int rc = 0;
	rc = sqlite3_open(database_name, &db);
	if (rc)
	{
		std::cerr << "Error opening SQLite3 database: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_close(db);
		return -1;
	}
	else
	{
		std::cout << "Opened " << database_name << std::endl;
	}
	return 0;
}

int DropTable(sqlite3 * db, const char* sql_stmt)
{
	if (sqlite3_exec(db, sql_stmt, 0, 0, 0) != SQLITE_OK) {
		std::cout << "SQLite can not drop table" << std::endl;
		sqlite3_close(db);
		return -1;
	}
	return 0;
}

int ExecuteSQL(sqlite3* db, const char* sql_stmt)
{
	int rc = 0;
	char* error = nullptr;
	rc = sqlite3_exec(db, sql_stmt, NULL, NULL, &error);
	if (rc)
	{
		std::cerr << "Error executing SQLite3 statement: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_free(error);
		return -1;
	}
	return 0;
}

int ShowTable(sqlite3* db, const char* sql_stmt)
{
	int rc = 0;
	char* error = nullptr;
	char** results = NULL;
	int rows, columns;
	sqlite3_get_table(db, sql_stmt, &results, &rows, &columns, &error);
	if (rc)
	{
		std::cerr << "Error executing SQLite3 query: " << sqlite3_errmsg(db) << std::endl << std::endl;
		sqlite3_free(error);
		return -1;
	}
	else
	{
		// Display Table
		for (int rowCtr = 0; rowCtr <= rows; ++rowCtr)
		{
			for (int colCtr = 0; colCtr < columns; ++colCtr)
			{
				// Determine Cell Position
				int cellPosition = (rowCtr * columns) + colCtr;

				// Display Cell Value
				std::cout.width(12);
				std::cout.setf(std::ios::left);
				std::cout << results[cellPosition] << " ";
			}

			// End Line
			std::cout << std::endl;

			// Display Separator For Header
			if (0 == rowCtr)
			{
				for (int colCtr = 0; colCtr < columns; ++colCtr)
				{
					std::cout.width(12);
					std::cout.setf(std::ios::left);
					std::cout << "~~~~~~~~~~~~ ";
				}
				std::cout << std::endl;
			}
		}
	}
	sqlite3_free_table(results);
	return 0;
}

int GetTable(sqlite3* db, const char* sql_stmt, char*** results_ptr, int* rows_ptr, int* columns_ptr)
{
    int rc = 0;
    char* error = nullptr;
    sqlite3_get_table(db, sql_stmt, results_ptr, rows_ptr, columns_ptr, &error);
    if (rc)
    {
        std::cerr << "Error executing SQLite3 query: " << sqlite3_errmsg(db) << std::endl << std::endl;
        sqlite3_free(error);
        return -1;
    }
    return 0;
}

void CloseDatabase(sqlite3* db)
{
	sqlite3_close(db);
}


bool IsTableExist(sqlite3* db, const std::string&table_name) {
	std::string sql_stmt = "SELECT COUNT(*) FROM sqlite_master where type ='table' and name ='" + table_name + "'";
	if (sqlite3_exec(db, sql_stmt.c_str(), 0,0,0) != SQLITE_OK) {
		std::cout << "Table " << table_name << " does not exist." << std::endl;
		return false;
	}
	return true;

}
