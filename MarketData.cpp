#include "MarketData.h"


int Pull_Populate_StockMap_Multi_Thread(const std::vector<string>& stocks, std::map<string, Stock*>& stockMap, std::string url_common, std::string start_date, std::string end_date, std::string api_token, int num_thread)
{
	vector<thread> threads;
	vector<vector<string>> sub_stocks_list;
	vector<map<string, Stock*>> sub_stockMap_list;
	int batch_size = (int)stocks.size() / num_thread + 1;

	for (int i = 0; i < num_thread; i++)
	{
		int start = i * batch_size;
		if (start > stocks.size()) start = int(stocks.size());
		int end = (i + 1) * batch_size;
		if (end > stocks.size()) end = int(stocks.size());

		auto start_ = start + stocks.begin();
		auto end_ = end + stocks.begin();

		vector<string> stocks_sliced(end_, start_);
		/*for (auto itr = stocks_sliced.begin(); itr != stocks_sliced.end(); itr++)
		{
			cout << (*itr) << "\t";
		}
		cout << endl;*/
		sub_stocks_list.emplace_back(stocks_sliced);

		map<string, Stock*> sub_stockMap;
		sub_stockMap_list.emplace_back(sub_stockMap);
	}

	for (int i = 0; i < num_thread; i++)
	{
		threads.emplace_back(thread(Pull_Populate_StockMap, ref(sub_stocks_list[i]), ref(sub_stockMap_list[i]), url_common, start_date, end_date, api_token));
	}

	for (thread& t : threads)
	{
		t.join();
	}

	for (auto& map : sub_stockMap_list)
	{
		for (auto itr = map.begin(); itr != map.end(); itr++)
		{
			stockMap[itr->first] = itr->second;
			itr->second = nullptr;
		}
	}
	return 0;
}


int Pull_Populate_StockMap(const std::vector<string>& stocks, std::map<string, Stock*>& stockMap, std::string url_common, std::string start_date, std::string end_date, std::string api_token)
{
	for (auto itr = stocks.begin(); itr != stocks.end(); itr++)
	{
		string url_request = url_common + *itr + ".US?" + "from=" + start_date + "&to=" + end_date + "&api_token=" + api_token + "&period=d&fmt=json";
		// cout << url_request << endl;
		string read_buffer; // read buffers for daily data
		Stock* stock = new Stock(*itr);

		// Retrieve daily trades into Stock object
		if (PullTrades_MultTry(url_request, read_buffer) == -1) return -1;
		if (PopulateDailyTrades(read_buffer, *stock) == -1) return -1;

		// cout << *stock << endl;
		// cout << "Successfully polulate " << *itr << "into stock map! " << endl;
		stockMap[*itr] = stock;
	}
	return 0;
}


int PullTrades_MultTry(const std::string& url_request, std::string& read_buffer)
{
	//global initiliation of curl before calling a function
	curl_global_init(CURL_GLOBAL_ALL);

	CURL* handle;

	// Store the result of CURL¨ªs webpage retrieval, for simple error checking.
	CURLcode result; // this is for error checking!
	int try_count = 0;

	while (true)
	{
		//creating session handle

		handle = curl_easy_init();

		if (!handle)
		{
			cout << "curl_easy_init failed" << endl;
			return -1;
		}

		curl_easy_setopt(handle, CURLOPT_URL, url_request.c_str());
		//adding a user agent
		curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:74.0) Gecko/20100101 Firefox/74.0");
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);

		// send all data to this function
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);

		// we pass our 'chunk' struct to the callback function
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, &read_buffer);

		result = curl_easy_perform(handle);

		if (result != CURLE_OK) {
			try_count += 1;
			cout << "Current Symbol trycount " << try_count << endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			continue;
		}

		break;
	}

	curl_easy_cleanup(handle);
	curl_global_cleanup();

	return 0;
}


int PullTrades_OneTry(const std::string& url_request, std::string& read_buffer) {
	//global initiliation of curl before calling a function
	curl_global_init(CURL_GLOBAL_ALL);

	CURL* handle;

	// Store the result of CURL¨ªs webpage retrieval, for simple error checking.
	CURLcode result; // this is for error checking!
	int try_count = 0;

	//creating session handle
	handle = curl_easy_init();

	if (!handle)
	{
		cout << "curl_easy_init failed" << endl;
		return -1;
	}
	curl_easy_setopt(handle, CURLOPT_URL, url_request.c_str());
	//adding a user agent
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:74.0) Gecko/20100101 Firefox/74.0");
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);
	// send all data to this function
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
	// we pass our 'chunk' struct to the callback function
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &read_buffer);
	result = curl_easy_perform(handle);
	if (result != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
		return -1;
	}
	if (read_buffer == "Ticker Not Found.") {
		return -2;
	}
	curl_easy_cleanup(handle);
	curl_global_cleanup();
	return 0;
}


int PopulateDailyTrades(const std::string& read_buffer, Stock& stock)
{
	//json parsing
	Json::CharReaderBuilder builder;
	Json::CharReader* reader = builder.newCharReader();
	Json::Value root;   // will contains the root value after parsing.
	string errors;

	bool parsingSuccessful = reader->parse(read_buffer.c_str(), read_buffer.c_str() + read_buffer.size(), &root, &errors);
	if (not parsingSuccessful)
	{
		// Report failures and their locations in the document.
		cout << "Failed to parse JSON" << endl << read_buffer << errors << endl;
		return -1;
	}
	else
	{
		// cout << "\nSuccess parsing json\n" << root << endl;
		// cout << "Success parsing json for " << stock.getSymbol() << endl;
		string date;
		float open, high, low, close, adjusted_close;
		int volume;
		for (Json::Value::const_iterator itr = root.begin(); itr != root.end(); itr++)
		{
			date = (*itr)["date"].asString();
			open = (*itr)["open"].asFloat();
			high = (*itr)["high"].asFloat();
			low = (*itr)["low"].asFloat();
			close = (*itr)["close"].asFloat();
			adjusted_close = (*itr)["adjusted_close"].asFloat();
			volume = (*itr)["volume"].asInt();
			TradeData aTrade(date, open, high, low, close, adjusted_close, volume);
			stock.addTrade(aTrade);
		}
		return 0;
	}
}