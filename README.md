# Pair Trading

This team project implemented a simple Pair Trading Strategy using C++ and SQLite on given stock pairs (see **PairTrading.txt**).

## Database Implementation Details
The database **PairTrading.db** has 4 tables: StockPairs, PairOnePrices, PairTwoPrices, PairPrices. They all have composite primary keys. PairPrices table has multiple foreign keys.

<div align=center><img width="300" height="500" src="https://user-images.githubusercontent.com/98775790/162598671-94da8fb4-4d08-4910-9acf-9535d62007ee.png"/></div>

## Pair Trading Strategy
1. Compute the standard deviation of ratio of the two adjusted closing stock prices in each pair from 1/1/2012 to 12/31/2021. Store this standard deviation in the StockPairs table.
2. Get Close1d1, Close2d1, Open1d2, Open2d2, Close1d2, and Close2d2, where Close1d1 and Close2d1 are the closing prices on day d – 1 for stocks 1 and 2, respectively, Open1d2, Open2d2 are the opening prices for day d.
3. Open Trade:
    -  If abs(Close1d1/Close2d1 – Open1d2/Open2d2) >= kσ, short the pair
    -  Else go long the pair
    -  N1 = 10,000 shares, traded at the price Open1d2
    -  N2 = N1 * (Open1d2/Open2d2), traded at the price Open2d2
    -  “Going short”: the first stock of the pair is short and the other is long
    -  “Going long”: the first stock of the pair is long and the other is short
    -  The parameter k could be defined by user in the pair trading program
4. Close Trade:
    - The open trades will be closed at the closing prices and P/L for the pair trade will be calculated as: (±N1 * [Close1d2 – Open1d2]) + (±N2 * [Close2d2 – Open2d2])
5. We used stock daily prices from 1/3/2022 to 3/6/2022 for back testing
6. The aggregated P/Ls were stored in the StockPairs table
<div align=center><img width="600" height="550" src="https://user-images.githubusercontent.com/98775790/162599151-97549705-7510-4f0f-a7f7-96dc92b830ee.png"/></div>

## Program Flow Chart

<div align=center><img width="800" height="460" src="https://user-images.githubusercontent.com/98775790/162599466-4540e29b-82d8-40e1-8421-1043b98c3abe.png"/></div>

## Additional Notes
- Some third party libraries (libcurl, json and sqlite) are required to run the program
- The program also supports manual testing
- Stock price data are downloaded from EOD database using multi-threading
- We solved the small bug of libcurl when doing multi-thread downloading on Windows PC (On Windows PC, when doing multi-thread downloading using libcurl sometimes the connection failed and we could not get the data on our buffer. In that case we needed to wait some time and rebuild the connection until we successfully get the correct data onto the buffer)

