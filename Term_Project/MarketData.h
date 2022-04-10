//
//  MarketData.h
//  FRE7831_Project
//
//  Created by 刘昶 on 2/12/22.
//	Modified by 谭浩喆 on 2/28/22

#pragma once
#include "Util.h"
#include "Stock.h"
#include "curl/curl.h"
#include "json/json.h"
#include <string>
#include <iostream>
#include <thread>

class Stock;

int Pull_Populate_StockMap_Multi_Thread(const std::vector<string>& stocks, std::map<string, Stock*>& stockMap, std::string url_common, std::string start_date, std::string end_date, std::string api_token, int num_thread);

int Pull_Populate_StockMap(const std::vector<string>& stocks, std::map<string, Stock*>& stockMap, std::string url_common, std::string start_date, std::string end_date, std::string api_token);

int PullTrades_MultTry(const std::string& url_request, std::string& read_buffer);

int PullTrades_OneTry(const std::string& url_request, std::string& read_buffer);

int PopulateDailyTrades(const std::string& read_buffer, Stock& stock);