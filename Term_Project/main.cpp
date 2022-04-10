#include "Stock.h"
#include "MarketData.h"
#include "Database.h"
#include "Util.h"
#include "curl/curl.h"
#include "json/json.h"
#include <assert.h>
#include <map>
#include <cmath>
#include <thread>
#include <set>
#include <algorithm>

using namespace std;

int main()
{
	//open the database
    int num_thread = 18;
	string database_name = "PairTrading.db";
	cout << "Opening PairTrading.db  . . . " << endl;
	sqlite3* db = NULL;
	if (OpenDatabase(database_name.c_str(), db) == -1)
		return -1;

	bool bCompleted = false;
	char selection;

	//global variables
	set<string> stocks1_set; // to prevent duplicates
	set<string> stocks2_set; // to prevent duplicates
    set<string> stocks_set; // to prevent duplicates
    vector<string> stocks;
	map<int, pair<string, string>> stockPairs; //indexed stock pairs
    map<string, Stock*> stockMap;
	string sql_CreateTable;
    
    //read stock pairs
    ifstream fin("PairTrading.txt");
    string line;
    int count = 1;
    
    while (getline(fin, line))
    {
        istringstream sin(line);
        string symbol;
        vector<string> symbols;
        while (getline(sin, symbol, ','))
        {
            symbols.push_back(symbol);
        }
        stocks_set.insert(symbols[0]);
        stocks1_set.insert(symbols[0]);
        stocks_set.insert(symbols[1]);
        stocks2_set.insert(symbols[1]);
        pair<int, pair<string, string>> index_pair = make_pair(count, make_pair(symbols[0], symbols[1]));
        stockPairs.insert(index_pair);
        count++;
    }
    
    // convert set to vector
    stocks.assign(stocks_set.begin(), stocks_set.end());
    
	while (!bCompleted)
    {
		cout << endl;
		cout << "Menu" << endl;
		cout << "========" << endl;
		cout << "A - Create and Populate Pair Table" << endl;
		cout << "B - Retrieve and Populate Historical Data for Each Stock" << endl;
		cout << "C - Create PairPrices Table" << endl;
		cout << "D - Calculate Volatility" << endl;
		cout << "E - Back Test" << endl;
		cout << "F - Calculate Profit and Loss for Each Pair" << endl;
		cout << "G - Manual Testing" << endl;
		cout << "H - Drop All the Tables" << endl;
		cout << "X - Exit" << endl << endl;

		cout << "Enter selection: ";
        if (!(cin >> selection))
        {
            cout << "Invalid Input, Please Reenter" << endl;
            cin.clear();
            while (cin.get() != '\n')
                // cin.get() is used for accessing character array. It includes white space characters. Generally, cin with an extraction operator (>>) terminates when whitespace is found. However, cin.get() reads a string with the whitespace.
                continue;
            // continue the while loop in the beginning
            continue;
        }
		cout << endl << endl;

		switch (selection)
        {
		case'a':
		case'A':
        {
			// Create StockPairs table
			cout << "Creating StockPairs table ..." << endl;
			sql_CreateTable = string("CREATE TABLE IF NOT EXISTS StockPairs ")
				+ "(id INT NOT NULL,"
				+ "Symbol1 CHAR(20) NOT NULL,"
				+ "Symbol2 CHAR(20) NOT NULL,"
				+ "Volatility FLOAT NOT NULL,"
				+ "Profit_Loss FLOAT NOT NULL,"
				+ "PRIMARY KEY(Symbol1, Symbol2));";
			if (ExecuteSQL(db, sql_CreateTable.c_str()) == -1)
				return -1;

			cout << "Created successfully!" << endl;

			//populate stock pairs into StockPairs table
			for (auto iter = stockPairs.begin(); iter != stockPairs.end(); iter++)
            {
				char sql_Insert[512];
				sprintf(sql_Insert, "INSERT INTO StockPairs(id, Symbol1, Symbol2, Volatility, Profit_Loss) VALUES(%d, \"%s\", \"%s\", %f, %f)",
					iter->first, ((iter->second).first).c_str(), ((iter->second).second).c_str(), 0.0, 0.0);
				if (ExecuteSQL(db, sql_Insert) == -1) return -1;
			}
			break;

		}

		case'b':
		case'B':
        {
			string sConfigFile = "config.csv";
			map<string, string> config_map = ProcessConfigData(sConfigFile);

			string url_common = config_map["url_common"];
			string start_date = config_map["start_date"];
			string end_date = config_map["end_date"];
			string api_token = config_map["api_token"];

			// Create PairOnePrices table
			cout << "Creating PairOnePrices table ..." << endl;
			sql_CreateTable = string("CREATE TABLE IF NOT EXISTS PairOnePrices ")
				+ "(Symbol CHAR(20) NOT NULL,"
				+ "Date CHAR(20) NOT NULL,"
				+ "Open REAL NOT NULL,"
				+ "High REAL NOT NULL,"
				+ "Low REAL NOT NULL,"
				+ "Close REAL NOT NULL,"
				+ "Adjusted_Close REAL NOT NULL,"
				+ "Volume INT NOT NULL,"
				+ "PRIMARY KEY(Symbol, Date));";
			if (ExecuteSQL(db, sql_CreateTable.c_str()) == -1)
				return -1;

			cout << "Created successfully!" << endl;


			// Create PairTwoPrices table
			cout << "Creating PairTwoPrices table ..." << endl;
			sql_CreateTable = string("CREATE TABLE IF NOT EXISTS PairTwoPrices ")
				+ "(Symbol CHAR(20) NOT NULL,"
				+ "Date CHAR(20) NOT NULL,"
				+ "Open REAL NOT NULL,"
				+ "High REAL NOT NULL,"
				+ "Low REAL NOT NULL,"
				+ "Close REAL NOT NULL,"
				+ "Adjusted_Close REAL NOT NULL,"
				+ "Volume INT NOT NULL,"
				+ "PRIMARY KEY(Symbol, Date));";
			if (ExecuteSQL(db, sql_CreateTable.c_str()) == -1)
				return -1;

			cout << "Created successfully!" << endl;

            if (Pull_Populate_StockMap_Multi_Thread(stocks, stockMap, url_common, start_date, end_date, api_token, num_thread) != 0)
                return -1;
            
			cout << "Successfully populate market data into Stock objects!" << endl;
            
            for (auto itr = stocks1_set.begin(); itr != stocks1_set.end(); itr++)
            {
                vector<TradeData> data = stockMap[*itr]->getTrades();
                cout << "Inserting daily data for " << *itr << " into table PairOnePrices ..." << endl << endl;
				// one-time insertion
                string sql_insert = string("INSERT INTO PairOnePrices(Symbol, Date, Open, High, Low, Close, Adjusted_close, Volume) VALUES ");
                char sub_sql_insert[512];
                for (auto i = data.begin(); i != data.end(); i++)
                {
                    sprintf(sub_sql_insert, "(\"%s\", \"%s\", %f, %f, %f, %f, %f, %d), ",
                    (*itr).c_str(), (*i).GetDate().c_str(), (*i).GetOpen(), (*i).GetHigh(), (*i).GetLow(), (*i).GetClose(), (*i).GetAdjustedClose(), (*i).GetVolume());
                    sql_insert += sub_sql_insert;
                }
                sql_insert = sql_insert.substr(0, sql_insert.length() - 2) + ";";
                if (ExecuteSQL(db, sql_insert.c_str()) == -1) return -1;
            }
            
            for (auto itr = stocks2_set.begin(); itr != stocks2_set.end(); itr++)
            {
                vector<TradeData> data = stockMap[*itr]->getTrades();
                cout << "Inserting daily data for " << *itr << " into table PairTwoPrices ..." << endl << endl;
				//one-time insertion
                string sql_insert = string("INSERT INTO PairTwoPrices(Symbol, Date, Open, High, Low, Close, Adjusted_close, Volume) VALUES ");
                char sub_sql_insert[512];
                for (auto i = data.begin(); i != data.end(); i++)
                {
                    sprintf(sub_sql_insert, "(\"%s\", \"%s\", %f, %f, %f, %f, %f, %d), ",
                    (*itr).c_str(), (*i).GetDate().c_str(), (*i).GetOpen(), (*i).GetHigh(), (*i).GetLow(), (*i).GetClose(), (*i).GetAdjustedClose(), (*i).GetVolume());
                    sql_insert += sub_sql_insert;
                }
                sql_insert = sql_insert.substr(0, sql_insert.length() - 2) + ";";
                if (ExecuteSQL(db, sql_insert.c_str()) == -1) return -1;
            }

			cout << "Successfully populate market data from Stock objects to SQL tables, PairOnePrices and PairTwoPrices!" << endl;

			break;
		
		}

		case'c':
		case'C':
        {
			// Create table PairPrices
			cout << "Creating PairPrices table ..." << endl;
			sql_CreateTable = string("CREATE TABLE IF NOT EXISTS PairPrices ")
				+ "(Symbol1 CHAR(20) NOT NULL,"
				+ "Symbol2 CHAR(20) NOT NULL,"
				+ "Date CHAR(20) NOT NULL,"
				+ "Open1 REAL NOT NULL,"
				+ "Close1 REAL NOT NULL,"
                + "Adjusted_Close1 REAL NOT NULL,"
				+ "Open2 REAL NOT NULL,"
				+ "Close2 REAL NOT NULL,"
                + "Adjusted_Close2 REAL NOT NULL,"
				+ "Profit_Loss REAL NOT NULL,"
				+ "PRIMARY KEY(Symbol1, Symbol2, Date),"
				+ "Foreign Key(Symbol1, Date) references PairOnePrices(Symbol, Date) "
				+ "ON DELETE CASCADE ON UPDATE CASCADE,"
				+ "Foreign Key(Symbol2, Date) references PairTwoPrices(Symbol, Date) "
				+ "ON DELETE CASCADE ON UPDATE CASCADE,"
				+ "Foreign Key(Symbol1, Symbol2) references StockPairs(Symbol1, Symbol2) "
				+ "ON DELETE CASCADE ON UPDATE CASCADE);";
			if (ExecuteSQL(db, sql_CreateTable.c_str()) == -1)
				return -1;
			cout << "Created successfully!" << endl;

			//Populate table PairPrices by querying other 3 tables
			string sql_PopulateTable = string("Insert into PairPrices ")
				+ "Select StockPairs.Symbol1 as Symbol1, StockPairs.Symbol2 as Symbol2, "
				+ "PairOnePrices.Date as Date, PairOnePrices.Open as Open1, PairOnePrices.Close as Close1, PairOnePrices.Adjusted_Close as Adjusted_Close1, "
				+ "PairTwoPrices.Open as Open2, PairTwoPrices.Close as Close2, PairTwoPrices.Adjusted_Close as Adjusted_Close2, 0 as Profit_Loss "
				+ "From StockPairs, PairOnePrices, PairTwoPrices "
				+ "Where (((StockPairs.Symbol1 = PairOnePrices.Symbol) and "
				+ "(StockPairs.Symbol2 = PairTwoPrices.Symbol)) and "
				+ "(PairOnePrices.Date = PairTwoPrices.Date)) "
				+ "ORDER BY Symbol1, Symbol2;";
			if (ExecuteSQL(db, sql_PopulateTable.c_str()) == -1)
                return -1;
			cout << "PairPrices Populated successfully!" << endl;
			break;
	
		}

		case'd':
		case'D':
        {
            string back_test_start_date = "2021-12-31";
            string sql_calculate_volatility_for_pair = string("Update StockPairs SET Volatility = ")
                + "(SELECT(AVG((Adjusted_Close1 / Adjusted_Close2) * (Adjusted_Close1/ Adjusted_Close2)) - AVG(Adjusted_Close1/Adjusted_Close2) * AVG(Adjusted_Close1/Adjusted_Close2)) as variance "
                + "FROM PairPrices "
                + "WHERE StockPairs.symbol1 = PairPrices.symbol1 AND StockPairs.symbol2 = PairPrices.symbol2 AND PairPrices.date <= \'"
                + back_test_start_date + "\');";
            if (ExecuteSQL(db, sql_calculate_volatility_for_pair.c_str()) == -1) return -1;
            cout << "StockPairs volatilily calculated successfully! " << endl;
            break;
        }

		case'e':
		case'E':
        {
            string back_test_start_date = "2021-12-31";
            double K;
            cout << "Please input a K: " << endl;
            if (!(cin >> K))
            {
                cout << "Invalid Input, Please enter a positive double" << endl;
                cin.clear();
                while (cin.get() != '\n')
                    // cin.get() is used for accessing character array. It includes white space characters. Generally, cin with an extraction operator (>>) terminates when whitespace is found. However, cin.get() reads a string with the whitespace.
                    continue;
                // continue the while loop in the beginning
                continue;
            }
            
            for (map<int, pair<string, string>>::const_iterator itr = stockPairs.begin(); itr != stockPairs.end(); itr++)
            {
                // Used to save data and index from db.table
                char** results;
                int rows, columns;
                
                // Other initializations
                vector<string> dates;
                pair<string, string> cur_pair = itr->second;
                StockPairPrices stockpairprices(cur_pair);
                stockpairprices.SetK(K);
                
                // Get volatility.
                char sql_get_volatility[512];
                sprintf(sql_get_volatility, "SELECT Volatility FROM StockPairs Where Symbol1 = \"%s\" And Symbol2 = \"%s\";", cur_pair.first.c_str(), cur_pair.second.c_str());
                GetTable(db, sql_get_volatility, &results, &rows, &columns);

				/*
				int size = sizeof(results) / sizeof(results[0]);
				for (int i = 0; i <= size; i++) {
					cout << results[i] << "\t";
				}
				cout << rows << "\t" << columns;
				cout << endl;
				*/
				


                string str_volatility = results[1];
                double variance = stod(str_volatility);
                stockpairprices.SetVolatility(variance);
                
                // Free Memory. Also can use: sqlite3_free_table(results);
                int length = (rows + 1) * columns;
                for (int i = 0; i < length; i++)
                {
                    delete [] results[i];
                    results[i] = nullptr;
                }
                
                // Get dates and pairprices
                char sql_get_dates_pairprices[512];
                sprintf(sql_get_dates_pairprices, "SELECT Date, Open1, Close1, Open2, Close2 FROM PairPrices Where Symbol1 = \"%s\" And Symbol2 = \"%s\" AND Date >= \"%s\";", cur_pair.first.c_str(), cur_pair.second.c_str(), back_test_start_date.c_str());
                GetTable(db, sql_get_dates_pairprices, &results, &rows, &columns);
                length = (rows + 1) * columns;
                
                // Populate stockpairprices
                // rows[0] are column names
                for (int i = 1; i <= rows; i++)
                {
                    string cur_date = results[i * columns];
                    string open1 = results[i * columns + 1];
                    string close1 = results[i * columns + 2];
                    string open2 = results[i * columns + 3];
                    string close2 = results[i * columns + 4];
                    dates.emplace_back(cur_date);
                    stockpairprices.SetDailyPairPrice(cur_date, PairPrice(stod(open1), stod(close1), stod(open2), stod(close2)));
                }
                
                // Free Memory. Also can use: sqlite3_free_table(results);
                length = (rows + 1) * columns;
                for (int i = 0; i < length; i++)
                {
                    delete [] results[i];
                    results[i] = nullptr;
                }
                
                // Calculate daily PNL and update it to db.
                double sigma = pow(variance, 0.5);
                for (int i = 1; i < dates.size(); i++)
                {
                    double PNL;
                    string last = dates[i-1];
                    string cur = dates[i];
                    double close1d1 = stockpairprices.GetDailyPrices()[last].dClose1;
                    double close2d1 = stockpairprices.GetDailyPrices()[last].dClose2;
                    double open1d2 = stockpairprices.GetDailyPrices()[cur].dOpen1;
                    double open2d2 = stockpairprices.GetDailyPrices()[cur].dOpen2;
                    double close1d2 = stockpairprices.GetDailyPrices()[cur].dClose1;
                    double close2d2 = stockpairprices.GetDailyPrices()[cur].dClose2;
                    int N1 = 10000;
                    int N2 = N1 * open1d2 / open2d2;
                    // Short 1, Long 2
                    if (abs(close1d1/close2d1 - open1d2/open2d2) > stockpairprices.GetK() * sigma)
                        PNL = N1 * (open1d2 - close1d2) - N2 * (open2d2 - close2d2);
                    // Long1, Short2
                    else
                        PNL = -N1 * (open1d2 - close1d2) + N2 * (open2d2 - close2d2);
                    // Update Profit_Loss
                    char sql_update_daily_pnl[512];
                    sprintf(sql_update_daily_pnl, "Update PairPrices Set Profit_Loss = %f Where Symbol1 = \"%s\" And Symbol2 = \"%s\" AND Date = \"%s\";", PNL, cur_pair.first.c_str(), cur_pair.second.c_str(), cur.c_str());
                    if (ExecuteSQL(db, sql_update_daily_pnl) == -1)
                        return -1;
                }
            }
            cout << "Daily PNL updated successfully! " << endl;
            break;
        }

		case'f':
		case'F':
        {
            string sql_update_total_pnl = string("Update StockPairs SET Profit_Loss = ")
                + "(SELECT SUM(Profit_Loss) "
                + "FROM PairPrices "
                + "WHERE StockPairs.Symbol1 = PairPrices.Symbol1 AND StockPairs.Symbol2 = PairPrices.Symbol2);";
            if (ExecuteSQL(db, sql_update_total_pnl.c_str()) == -1)
                return -1;
            cout << "Total PNL for each pair updated successfully! " << endl;;
            break;
        }

		case'g':
		case'G':
        {
            // First we must know the stock names we already have in the database
            char** results;
            int rows, columns;
            char sql_get_stocks[512];
            vector<string> stock1sInDb, stock2sInDb;
            // Find the stocks in the PairOnePrices table
            sprintf(sql_get_stocks, "SELECT DISTINCT(Symbol) from PairOnePrices;");
            GetTable(db, sql_get_stocks, &results, &rows, &columns); // we would have a n row 1 col result
            assert(columns == 1);
            for (int i = 1; i <= rows; i++) //  i = 1 means we skip the header.
            {
                string tikerName = results[i * columns];
                stock1sInDb.push_back(tikerName);
            }
            // Free Memory
            sqlite3_free_table(results);
            // Find the stocks in the PairTwoPrices table
            sprintf(sql_get_stocks, "SELECT DISTINCT(Symbol) from PairTwoPrices;");
            GetTable(db, sql_get_stocks, &results, &rows, &columns); // we would have a n row 1 col result
            assert(columns == 1);
            for (int i = 1; i <= rows; i++) //  i = 1 means we skip the header.
            {
                string tikerName = results[i * columns];
                stock2sInDb.push_back(tikerName);
            }
            // Free Memory
            sqlite3_free_table(results);

			// Secondly, we need to obtain the existing stock paris in the StockPairs table.
			vector<pair<string, string>> stockPairsInDb;
			char sql_get_pairs[512];
			sprintf(sql_get_pairs, "SELECT DISTINCT Symbol1, Symbol2 from StockPairs;");
			GetTable(db, sql_get_pairs, &results, &rows, &columns); // we would have a n row 2 col result
			assert(columns == 2);
			for (int i = 1; i <= rows; i++) //  i = 1 means we skip the header.
			{
				string tikerName1 = results[i * columns];
				string tikerName2 = results[i * columns + 1];
				stockPairsInDb.push_back(make_pair(tikerName1, tikerName2));
			}
			// Free Memory
			sqlite3_free_table(results);

			// Now lets begin our manual test...
			// Condition1, whether the 1st/2nd symbol can be found in PairOnePrices/PairTwoPrices table
			// If yes, move on to the next step
			// If not, we need to inject the data for the missing stock.
			// check symbol1
			string symbol1, symbol2;
			cout << "Please Give the symbol name of the first stock you want to trade:" << endl;
			cin >> symbol1;
			//convert lower case ticker to upper case ticker
			transform(symbol1.begin(), symbol1.end(), symbol1.begin(), ::toupper);
			if (std::count(stock1sInDb.begin(), stock1sInDb.end(), symbol1) == 0) {
				cout << "Symbol not found in PairOneTable, now try to obtain data and update PairOneTable..." << endl;
				string sConfigFile = "config.csv";
				map<string, string> config_map = ProcessConfigData(sConfigFile);
				string url_common = config_map["url_common"];
				string start_date = config_map["start_date"];
				string end_date = config_map["end_date"];
				string api_token = config_map["api_token"];
				string url_request = url_common + symbol1 + ".US?" + "from=" + start_date + "&to=" + end_date + "&api_token=" + api_token + "&period=d&fmt=json";

				string read_buffer; // read buffers for daily data
				Stock stock(symbol1); // storage dailyTrade data of symbol1

				// Retrieve daily trades into Stock object
				int requestResult = PullTrades_OneTry(url_request, read_buffer);
				if (requestResult == -1) return -1;
				else if (requestResult == -2) {
					cout << "Symbol1 name not found in EOD database." << endl;
					cout << "Please find the correct symbol name and come back to manual test again..." << endl;
					break;
				}
				if (PopulateDailyTrades(read_buffer, stock) == -1) return -1;

				// Insert data once
				cout << "Inserting daily data for " << symbol1 << " into table PairOnePrices ..." << endl << endl;
				vector<TradeData> dailyTrade = stock.getTrades();
				string sql_insert = string("INSERT INTO PairOnePrices (Symbol, Date, Open, High, Low, Close, Adjusted_close, Volume) VALUES ");
				char sql_insert_values[512];
				for (auto itr = dailyTrade.begin(); itr != dailyTrade.end(); itr++)
				{
					sprintf(sql_insert_values, "(\"%s\", \"%s\", %f, %f, %f, %f, %f, %d), ",
						symbol1.c_str(), (*itr).GetDate().c_str(), (*itr).GetOpen(), (*itr).GetHigh(), (*itr).GetLow(), (*itr).GetClose(), (*itr).GetAdjustedClose(), (*itr).GetVolume());
					sql_insert += sql_insert_values;
				}
				sql_insert = sql_insert.substr(0, sql_insert.length() - 2) + ";";
				if (ExecuteSQL(db, sql_insert.c_str()) == -1) return -1;
			}
			else { cout << "symbol1 found in PairOnePrices" << endl; }
			// check symbol2
			cout << "Please Give the symbol name of the second stock you want to trade:" << endl;
			cin >> symbol2;
			// convert lower case ticker to upper case ticker
			transform(symbol2.begin(), symbol2.end(), symbol2.begin(), ::toupper);
			if (std::count(stock2sInDb.begin(), stock2sInDb.end(), symbol2) == 0) {
				cout << "Symbol not found in PairTwoTable, now try to obtain data and update PairTwoTable..." << endl;
				string sConfigFile = "config.csv";
				map<string, string> config_map = ProcessConfigData(sConfigFile);
				string url_common = config_map["url_common"];
				string start_date = config_map["start_date"];
				string end_date = config_map["end_date"];
				string api_token = config_map["api_token"];
				string url_request = url_common + symbol2 + ".US?" + "from=" + start_date + "&to=" + end_date + "&api_token=" + api_token + "&period=d&fmt=json";

				string read_buffer; // read buffers for daily data
				Stock stock(symbol2); // storage dailyTrade data of symbol2

				// Retrieve daily trades into Stock object
				int requestResult = PullTrades_OneTry(url_request, read_buffer);
				if (requestResult == -1) return -1;
				else if (requestResult == -2) {
					cout << "Symbol2 name not found in EOD database." << endl;
					cout << "Please find the correct symbol name and come back to manual test again..." << endl;
					break;
				}  
				if (PopulateDailyTrades(read_buffer, stock) == -1) return -1;

				// Insert data once
				cout << "Inserting daily data for " << symbol2 << " into table PairTwoPrices ..." << endl << endl;
				vector<TradeData> dailyTrade = stock.getTrades();
				string sql_insert = string("INSERT INTO PairTwoPrices (Symbol, Date, Open, High, Low, Close, Adjusted_close, Volume) VALUES ");
				char sql_insert_values[512];
				for (auto itr = dailyTrade.begin(); itr != dailyTrade.end(); itr++)
				{
					sprintf(sql_insert_values, "(\"%s\", \"%s\", %f, %f, %f, %f, %f, %d), ",
						symbol2.c_str(), (*itr).GetDate().c_str(), (*itr).GetOpen(), (*itr).GetHigh(), (*itr).GetLow(), (*itr).GetClose(), (*itr).GetAdjustedClose(), (*itr).GetVolume());
					sql_insert += sql_insert_values;
				}
				sql_insert = sql_insert.substr(0, sql_insert.length() - 2) + ";";
				if (ExecuteSQL(db, sql_insert.c_str()) == -1) return -1;
			}
			else { cout << "symbol2 found in PairTwoPrices" << endl; }

			// Check whether the stockPair exists already.
			bool pair_exist = false;
			string back_test_start_date = "2021-12-31";
			for (auto iter = stockPairsInDb.begin(); iter != stockPairsInDb.end(); iter++) {
				if ((*iter) == make_pair(symbol1, symbol2)) {
					pair_exist = true;
					break;
				}
			}

			// If the pair does not exist, we need to do the D,E,F step
			if (!pair_exist) {
				// Delete redundant data in case of previous false trail
				char sql_Delete[512];
				sprintf(sql_Delete, "DELETE FROM PairPrices where (PairPrices.Symbol1 = \"%s\") and (PairPrices.Symbol2 = \"%s\")", symbol1.c_str(), symbol2.c_str());
				if (ExecuteSQL(db, sql_Delete) == -1) return -1;
				cout << "PairPrices redundant Data Deleted successfully!" << endl;

				// Add 1 row to StockPairs table
				int numExistPairs = stockPairsInDb.size();
				char sql_Insert[1024];
				sprintf(sql_Insert, "INSERT INTO StockPairs(id, Symbol1, Symbol2, Volatility, Profit_Loss) VALUES(%d, \"%s\", \"%s\", %f, %f)",
					numExistPairs + 1, symbol1.c_str(), symbol2.c_str(), 0.0, 0.0);
				if (ExecuteSQL(db, sql_Insert) == -1) return -1;
				cout << "StockPairs Update successfully!" << endl;

				// Add multiple rows to PairPrices table
				sprintf(sql_Insert, "Insert into PairPrices\n" \
					"Select StockPairs.Symbol1 as Symbol1, StockPairs.Symbol2 as Symbol2,\n" \
					"PairOnePrices.Date as Date, PairOnePrices.Open as Open1, PairOnePrices.Close as Close1, PairOnePrices.Adjusted_Close as Adjusted_Close1,\n" \
					"PairTwoPrices.Open as Open2, PairTwoPrices.Close as Close2, PairTwoPrices.Adjusted_Close as Adjusted_Close2, 0 as Profit_Loss\n" \
					"From StockPairs, PairOnePrices, PairTwoPrices\n" \
					"Where (StockPairs.Symbol1 = PairOnePrices.Symbol)\n" \
					"AND (StockPairs.Symbol2 = PairTwoPrices.Symbol)\n" \
					"AND (PairOnePrices.Date = PairTwoPrices.Date)\n" \
					"AND (PairOnePrices.Symbol = \"%s\")\n" \
					"AND (PairTwoPrices.Symbol = \"%s\")\n",
					symbol1.c_str(), symbol2.c_str());
				if (ExecuteSQL(db, sql_Insert) == -1)
					return -1;
				cout << "PairPrices Update successfully!" << endl;

				// Calculate vol
				char sql_calculate_volatility_for_pair[1024];
				sprintf(sql_calculate_volatility_for_pair, "Update StockPairs SET volatility =(\n" \
					"SELECT (AVG((p1.Adjusted_close/p2.Adjusted_close)*(p1.Adjusted_close/p2.Adjusted_close)) - AVG(p1.Adjusted_close/p2.Adjusted_close)*AVG(p1.Adjusted_close/p2.Adjusted_close))\n" \
					"FROM PairPrices pp\n" \
					"LEFT JOIN PairOnePrices p1, PairTwoPrices p2\n" \
					"ON p1.Date = pp.Date and p2.Date = pp.Date and p1.Symbol = pp.Symbol1 and p2.Symbol = pp.Symbol2\n" \
					"WHERE StockPairs.symbol1 = pp.symbol1\n" \
					"AND StockPairs.symbol2 = pp.symbol2\n" \
					"AND pp.Date <= \"%s\"\n" \
					")\n" \
					"WHERE StockPairs.symbol1 = \"%s\"\n" \
					"AND StockPairs.symbol2 = \"%s\"\n",
					back_test_start_date.c_str(), symbol1.c_str(), symbol2.c_str());
				if (ExecuteSQL(db, sql_calculate_volatility_for_pair) == -1)
					return -1;
				cout << "PairPrice vol Update successfully!" << endl;

				// BackTest
				// Set k
				int K;
				cout << "Please input a K: " << endl;
				if (!(cin >> K))
				{
					cout << "Invalid Input, Please enter a positive interger" << endl;
					cin.clear();
					while (cin.get() != '\n')
						continue;
					continue;
				}
				// Initializations
				char** results;
				int rows, columns;
				vector<string> dates;
				pair<string, string> cur_pair = make_pair(symbol1, symbol2);
				StockPairPrices stockpairprices(cur_pair);
				stockpairprices.SetK(K);
				// Get volatility.
				char sql_get_volatility[512];
				sprintf(sql_get_volatility, "SELECT Volatility FROM StockPairs Where Symbol1 = \"%s\" And Symbol2 = \"%s\";", cur_pair.first.c_str(), cur_pair.second.c_str());
				GetTable(db, sql_get_volatility, &results, &rows, &columns);
				string str_volatility = results[1];
				double variance = stod(str_volatility);
				stockpairprices.SetVolatility(variance);
				// Free Memory.
				sqlite3_free_table(results);
				// Get dates and pairprices
				char sql_get_dates_pairprices[512];
				sprintf(sql_get_dates_pairprices, "SELECT Date, Open1, Close1, Open2, Close2 FROM PairPrices Where Symbol1 = \"%s\" And Symbol2 = \"%s\" AND Date >= \"%s\";", cur_pair.first.c_str(), cur_pair.second.c_str(), back_test_start_date.c_str());
				GetTable(db, sql_get_dates_pairprices, &results, &rows, &columns);
				int length = (rows + 1) * columns;
				// Populate stockpairprices
				// rows[0] are column names
				for (int i = 1; i <= rows; i++)
				{
					string cur_date = results[i * columns];
					string open1 = results[i * columns + 1];
					string close1 = results[i * columns + 2];
					string open2 = results[i * columns + 3];
					string close2 = results[i * columns + 4];
					dates.emplace_back(cur_date);
					stockpairprices.SetDailyPairPrice(cur_date, PairPrice(stod(open1), stod(close1), stod(open2), stod(close2)));
				}
				// Free Memory.
				sqlite3_free_table(results);
				// Calculate daily PNL and update it to db.
				double sigma = pow(variance, 0.5);
				for (int i = 1; i < dates.size(); i++)
				{
					double PNL;
					string last = dates[i - 1]; // loop starts from the second day
					string cur = dates[i];
					double close1d1 = stockpairprices.GetDailyPrices()[last].dClose1;
					double close2d1 = stockpairprices.GetDailyPrices()[last].dClose2;
					double open1d2 = stockpairprices.GetDailyPrices()[cur].dOpen1;
					double open2d2 = stockpairprices.GetDailyPrices()[cur].dOpen2;
					double close1d2 = stockpairprices.GetDailyPrices()[cur].dClose1;
					double close2d2 = stockpairprices.GetDailyPrices()[cur].dClose2;
					int N1 = 10000;
					int N2 = N1 * open1d2 / open2d2;
					// Short 1, Long 2
					if (abs(close1d1 / close2d1 - open1d2 / open2d2) > stockpairprices.GetK() * sigma)
						PNL = N1 * (open1d2 - close1d2) - N2 * (open2d2 - close2d2);
					// Long1, Short2
					else
						PNL = -N1 * (open1d2 - close1d2) + N2 * (open2d2 - close2d2);
					// Update Profit_Loss in PairPrices table
					char sql_update_daily_pnl[512];
					sprintf(sql_update_daily_pnl, "Update PairPrices Set Profit_Loss = %f Where Symbol1 = \"%s\" And Symbol2 = \"%s\" AND Date = \"%s\";", PNL, cur_pair.first.c_str(), cur_pair.second.c_str(), cur.c_str());
					if (ExecuteSQL(db, sql_update_daily_pnl) == -1)
						return -1;
				}
				cout << "Daily PNL updated successfully in table PairPrices! " << endl;
				// Update Profit_Loss in StockPair table
				char sql_update_total_pnl[512];
				sprintf(sql_update_total_pnl, "Update StockPairs SET Profit_Loss = (\n" \
					"SELECT SUM(Profit_Loss)\n" \
					"FROM PairPrices\n" \
					"WHERE StockPairs.Symbol1 = PairPrices.Symbol1 AND StockPairs.Symbol2 = PairPrices.Symbol2\n" \
					")\n" \
					"WHERE StockPairs.symbol1 = \"%s\"\n" \
					"AND StockPairs.symbol2 = \"%s\"\n",
					symbol1.c_str(), symbol2.c_str());
				if (ExecuteSQL(db, sql_update_total_pnl) == -1)
					return -1;
				cout << "Total PNL for each pair updated successfully in table StockPairs!" << endl;

				// Result Display
				cout << "Retrieving values in table PairPrices ..." << endl;
				char sql_Select_pairPrice[512];
				sprintf(sql_Select_pairPrice, "SELECT * FROM PairPrices WHERE PairPrices.symbol1 = \"%s\" AND PairPrices.symbol2 = \"%s\" AND PairPrices.Date >= \"%s\"",
					symbol1.c_str(), symbol2.c_str(), back_test_start_date.c_str());
				if (ShowTable(db, sql_Select_pairPrice) == -1)
					return -1;
				cout << "Retrieving values in table StockPairs ..." << endl;
				char sql_Select_stockPair[512];
				sprintf(sql_Select_stockPair, "SELECT * FROM StockPairs WHERE StockPairs.symbol1 = \"%s\" AND StockPairs.symbol2 = \"%s\"",
					symbol1.c_str(), symbol2.c_str());
				if (ShowTable(db, sql_Select_stockPair) == -1)
					return -1;
			}
			else { // Pair existed!
				//Result Display
				cout << "Retrieving values in table PairPrices ..." << endl;
				char sql_Select_pairPrice[512];
				sprintf(sql_Select_pairPrice, "SELECT * FROM PairPrices WHERE PairPrices.symbol1 = \"%s\" AND PairPrices.symbol2 = \"%s\" AND PairPrices.Date >= \"%s\"",
					symbol1.c_str(), symbol2.c_str(), back_test_start_date.c_str());
				if (ShowTable(db, sql_Select_pairPrice) == -1)
					return -1;
				cout << "Retrieving values in table StockPairs ..." << endl;
				char sql_Select_stockPair[512];
				sprintf(sql_Select_stockPair, "SELECT * FROM StockPairs WHERE StockPairs.symbol1 = \"%s\" AND StockPairs.symbol2 = \"%s\"",
					symbol1.c_str(), symbol2.c_str());
				if (ShowTable(db, sql_Select_stockPair) == -1)
					return -1;
			}
			break;
        }

		case'h':
		case'H':
        {
            // Drop the table if exists
            cout << "Drop StockPairs table if exists" << endl;
            string sql_DropaTable = "DROP TABLE IF EXISTS StockPairs";
            if (DropTable(db, sql_DropaTable.c_str()) == -1)
                return -1;

            cout << "Drop PairTwoPrices table if exists" << endl;
            sql_DropaTable = "DROP TABLE IF EXISTS PairTwoPrices";
            if (DropTable(db, sql_DropaTable.c_str()) == -1)
                return -1;

            cout << "Drop PairOnePrices table if exists" << endl;
            sql_DropaTable = "DROP TABLE IF EXISTS PairOnePrices";
            if (DropTable(db, sql_DropaTable.c_str()) == -1)
                return -1;

            cout << "Drop PairPrices table if exists" << endl;
            sql_DropaTable = "DROP TABLE IF EXISTS PairPrices";
            if (DropTable(db, sql_DropaTable.c_str()) == -1)
                return -1;
            
            cout << "All Tables dropped successfully! ";
            break;
        }

		case'x':
		case'X':
        {
			bCompleted = true;
			CloseDatabase(db);

			// Free memory
			for (auto itr = stockMap.begin(); itr != stockMap.end(); itr++) {
				delete itr->second;
				itr->second = nullptr;
			}

			cout << "Exit the loop." << endl;
			break;
        }
                
        default:
        {
            cout << "Invalid Input" << endl;
            break;
		}
        }
	}

	CloseDatabase(db);

	return 0;
}
