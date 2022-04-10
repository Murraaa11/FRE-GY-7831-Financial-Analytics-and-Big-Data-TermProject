#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<map>
using namespace std;

class TradeData 
{
private:
	string sDate;
	double dOpen;
	double dHigh;
	double dLow;
	double dClose;
	double dAdjClose;
	int lVolume;
public:
	TradeData() :sDate(""), dOpen(0), dHigh(0), dLow(0), dClose(0), dAdjClose(0), lVolume(0) {}
	TradeData(string sDate_, double dOpen_, double dHigh_, double dLow_, double dClose_, double dAdjClose_, long lVolume_) :
		sDate(sDate_), dOpen(dOpen_), dHigh(dHigh_), dLow(dLow_), dClose(dClose_), dAdjClose(dAdjClose_), lVolume(lVolume_) {}
	TradeData(const TradeData& TradeData) : sDate(TradeData.sDate), dOpen(TradeData.dOpen), dHigh(TradeData.dHigh), dLow(TradeData.dLow),
		dClose(TradeData.dClose), dAdjClose(TradeData.dAdjClose), lVolume(TradeData.lVolume) {}
	TradeData operator=(const TradeData& TradeData) {
		sDate = TradeData.sDate;
		dOpen = TradeData.dOpen;
		dHigh = TradeData.dHigh;
		dLow = TradeData.dLow;
		dClose = TradeData.dClose;
		dAdjClose = TradeData.dAdjClose;
		lVolume = TradeData.lVolume;

		return *this;
	}
	string GetDate() const { return sDate; }
	double GetOpen() const { return dOpen; }
	double GetHigh() const { return dHigh; }
	double GetLow() const { return dLow; }
	double GetClose() const { return dClose; }
	double GetAdjustedClose() const { return dAdjClose; }
	int GetVolume() const { return lVolume; }

	friend ostream& operator << (ostream& out, const TradeData& t)
	{
		out << "Date: " << t.sDate << " Open: " << t.dOpen << " High: " << t.dHigh << " Low: " << t.dLow
			<< " Close: " << t.dClose << " Adjusted Close: " << t.dAdjClose << " Volume: " << t.lVolume << endl;
		return out;
	}

};

class Stock 
{
private:
	string sSymbol;
	vector<TradeData> trades;

public:
	Stock() :sSymbol("") {}
	Stock(string sSymbol_, const vector<TradeData> trades_) :sSymbol(sSymbol_), trades(trades_) {}
	Stock(string sSymbol_) : sSymbol(sSymbol_) {}
	Stock(const Stock& stock) : sSymbol(stock.sSymbol), trades(stock.trades) {}
	Stock operator=(const Stock& stock) {
		sSymbol = stock.sSymbol;
		trades = stock.trades;

		return *this;
	}
	void addTrade(const TradeData& trade) { trades.push_back(trade); }
	string getSymbol() { return sSymbol; }
	const vector<TradeData>& getTrades() const { return trades; }

	friend ostream& operator<<(ostream& ostr, const Stock& stock) {
		ostr << "Symbol: " << stock.sSymbol << endl;
		for (auto itr = stock.trades.begin(); itr != stock.trades.end(); itr++) {
			ostr << *itr;
		}
		return ostr;
	}

};

struct PairPrice {
	double dOpen1;
	double dClose1;
	double dOpen2;
	double dClose2;
	double dProfit_Loss;
	PairPrice():dOpen1(0), dClose1(0), dOpen2(0), dClose2(0), dProfit_Loss(0){}
	PairPrice(double dOpen1_, double dClose1_, double dOpen2_, double dClose2_): 
		dOpen1(dOpen1_), dClose1(dClose1_), dOpen2(dOpen2_), dClose2(dClose2_), dProfit_Loss(0) {}
};

class StockPairPrices
{
private:
	pair<string, string> stockPair;
	double volatility;
	double k;
	map<string, PairPrice> dailyPairPrices;
public:
	StockPairPrices() { volatility = 0; k = 0; }
	StockPairPrices(pair<string, string> stockPair_) { stockPair = stockPair_; volatility = 0; k = 0; }
	void SetDailyPairPrice(string sDate_, PairPrice pairPrice_) {
		dailyPairPrices.insert(pair<string, PairPrice>(sDate_, pairPrice_));
	}
	void SetVolatility(double volatility_) {volatility = volatility_;}
	void SetK(double k_) { k = k_; }
	void UpdateProfitLoss(string sDate_, double dProfitLoss_) {
		dailyPairPrices[sDate_].dProfit_Loss = dProfitLoss_;
	}
	pair<string, string> GetStockPair() const {
		return stockPair;
	}
	map<string, PairPrice> GetDailyPrices() const {
		return dailyPairPrices;
	}
	double GetVolatility() const {
		return volatility;
	}
	double GetK() const {
		return k;
	}
};
