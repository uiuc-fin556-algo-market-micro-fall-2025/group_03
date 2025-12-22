# FIN556 Group 3 Project Report

## Team Members

### Ellis Irwin (Team Leader)

Email: ellisi2@illinois.edu

LinkedIn: https://www.linkedin.com/in/ellis-irwin/

I am a Master's student studying Computer Science at the University of Illinois Urbana-Champaign with a strong interest in
low-level systems programming and computer architecture. I have previously completed a software engineering internship at
Millennium Management where I learned to apply my computer science skills to financial markets.

### David Adam

Email: dadam7@illinois.edu

LinkedIn:[](https://www.linkedin.com/in/david-adam5/)

I am a senior Systems Engineering student with a secondary field in Computer Science (Financial Technology) at the University of Illinois at Urbana-Champaign. I have a strong interest in automation and business workflow optimization and my previous professional experiences have consisted of working as an AI & Workflow Development Intern at Employment BOOST and as an IT & Automation Intern at Consolidated Hospitality Supplies.

### Dev Desai

Email: ddesa4@illinois.edu

LinkedIn: https://www.linkedin.com/in/devvdesai/

I am a senior studying civil engineering at the University of Illinois at Urbana-Champaign. Currently, I am focused on 
structural engineering with a secondary focus in financial engineering. My recent professional experiences include interning 
at Richard as a Project Engineering Intern and as an Analyst Intern at Lionchase North America.

## Preparation

### Initial Strategy Development

When we initially met, we had come up with multiple ideas on how we could tackle this project. The one that stood out the most 
was trading SPY based on underlying stock movements and taking advantage of movement delays between tracking assets. These 
two ideas seemed very similar, and those are the ideas that are presented in this report.

Additionally, we also thought about another overnight drift algorithm, but we couldn’t come up with an idea that was different 
than the idea that was talked about in this class. We also considered the possibility of utilizing an LLM, with AI becoming a 
large topic in this class, and using those LLMs to optimize simple trading strategies such as the moving average crossover trade, 
or simple “read the news” trades on mega-cap stocks. Finally, the last idea that we discussed was an earnings drift trade.

While we thought some of these more complex strategies would be cool, we moved away from them as none of us have super advanced
math knowledge or coursework. Additionally, having never worked with Strategy Studio before, these strategies may have been too
ambitious as we also needed to spend time learning how to use Strategy Studio. For this reason, we decided to stick to more basic 
index arbitrage and direct arbitrage strategies.

### Strategy Studio Familiarization

In parallel with our initial strategy brainstorming and development, we spent a lot of the initial weeks of the project on
playing around within Strategy Studio to understand how to effectively use it and all of its capabilities. 

Starting by only having access to the command line was a challenge but gave us valuable experience working with systems that
only have command line access that we will likely see throughout our time in industry. This initial testing on the command line
gave us experience with the command line and allowed us to get a basic understanding of Strategy Studio.

### IEX Market Data Parser

In addition to theorizing mathematically about strategies and learning how to build them out in Strategy Studio, we also
had to work with the IEX market data PCAPs and the given parser to successfully parse the data files that would be passed into
Strategy Studio.

We spent some time looking through the given parser and making some tweaks so that the parser correctly parsed the tickers
that we needed for our strategies.

## Strategies

### Index Arbitrage

The first strategy that we implemented was an index arbitrage strategy for SPY (that we later repurposed to compare with DIA). 

Due to none of us having any significant Strategy Studio experience, we started with a very basic strategy that would be easiest to
implement and build on, making it more complex over time. Each of the following optimizations were built out one at a time to
slowly progress and improve our strategy.

Our first step was to receive data for just SPY and MSFT, and whenever there was a trade on MSFT, we would replicate the same
trade on SPY. At a basic level this is correct, if people are buying MSFT, and MSFT is a holding within SPY, then SPY should
go up, meaning we should purchase SPY. However, this is obviously a crude approximation. 

The next optimization we took was to use the last trade price as the price of the last stock and use the largest holdings
within SPY to calculate a theoretical price for SPY using weighted averages. For example, we started with the following: 
R_under = (7.38% * NVDA) + (6.25% * MSFT) + (3.87% * AMZN) + (2.55% * GOOG). We can then also calculate 
R_spy = (7.38% + 6.25% + 3.87% + 2.55%) * SPY. Theoretically, R_under = R_spy, if R_under is larger, we can submit a buy order
and if it's smaller, we can submit a sell order to capitalize on discrepancies.

The next optimization was to use z-scores instead of the direct price difference of R_under and R_spy. We could calculate
delta = R_under - R_spy. If this delta was positive, that is a buy signal, and if it's negative, it's a sell signal. We could then
maintain a rolling window of previous delta values and calculate the z-score of the newest delta in relation to previous ones.
If this delta is above our z-score threshold, then we would execute our buy or sell order.

We next realized that our pricing of instruments by just using last trade price was not very accurate, so we should use some
price between the best bid and ask. For this, we decided to update our price for each stock on every OnTopQuote event, and
we calculated the weighted mid-price of the stock. This gave us a much more accurate price representation of each instrument.

Next, we implemented dynamic position sizing. Previously, whenever our z-score was above the threshold, we placed an order of
the same size. This meant that a z-score of 1.5 and 10 would both take the same position. We modified this to scale the
position size with the magnitude of the z-score, allowing us to capitalize more on high z-scores and reduce risk on lower
z-scores.

Another small tweak we changed was the type of orders we were placing. Instead of placing market orders, we used "market
with leftover as limit" orders to prevent our trades from climbing down from the top of book to unfavorable prices.

Lastly, we used the IEX market data parser mentioned in the previous section to parse more of the holdings of SPY. This
gave us a larger percentage of the SPY basket, giving us a more accurate R_under and therefore better deltas.

#### SPY Index Arbitrage

We implemented the index arbitrage strategy outlined above for SPY with the results and analysis below.

##### Results

![](SPY_pnl_6month.png)

##### Analysis

Ignoring the the last month, our strategy performed quite well, bouncing around 0 PnL and sometimes went positive. This shows we
have at least a decent algorithm that reliably calculates R_under and provides us with good arbitrage opportunities. 

We do however see the returns go very negative and very spiky in June. We are not entirely sure as to why this happens but
one potential fix for this we thought could be beneficial would be to add stop losses. This would liquidate positions before
large price changes, which could potentially eliminate these PnL spikes and improve returns.

#### DIA Index Arbitrage

For DIA index arbitrage, we copied the code from our SPY index arb and just modified the instrument used and their weights
to represent a DIA arbitrage.

##### Results

![](DIA_pnl_6month.png)

##### Analysis

In our results here for DIA, we see much less readable results, with it jumping up and down incredible frequently.

This jumpiness could be due to the lower liquidity of DIA. Additionally, we were unable to get very accurate weights to use
for our arbitrage. Since DIA is price weighted as opposed to market cap weighted, the percentage makeup of DIA is much more
volatile. I was able to find some weights of holdings from previous years and it was clear that the weights were significantly
different to the weights we see today. We unfortunately could not find any weights from near the backtesting period that we
were running, so it is very likely that the weights we used are not representative of the actual composition of DIA

Due to the price weightedness of DIA and its issues explained above, we believe that running an index arb on SPY or some other 
market cap weighted ETF should be done and is much more effective.

### Direct Arbitrage

#### SPY/QQQ Arbitrage

##### Results

![](SPY_QQQ_Direct_pnl_6month.png)

#### Analysis


## Challenges

We ran into numerous challenges throughout our development process. Here are some of the biggest ones we faced with some of the
ways we solved them:
- Unfamiliarity with Linux command line
    - ChatGPT was a significant support initially but through repetition, familiarity grew and our confidence grew along with it
- Inability to build and run strategies in Strategy Studio
    - Analyze and research the Strategy Studio documentation and header files, as well as updating the bash scripts to correctly build and run our strategies
- FIN556 VM constraints
    - We had to symlink all of our files to our student_work directory to effectively use all of our resources, such as the text_tick_data
- Issues correctly plotting PnL over time
    - Had to code and use ChatGPT to correctly display dates on the x-axis while maintaining readability in the plots for future report work

There are likely many other large challenges we ran into that I missed in addition to numerous smaller challenges that we had
to troubleshoot and overcome.

## Skills & Lessons Learned

This project proved to be incredibly rewarding as we were able to work in a production environment with access to production
level software like Strategy Studio. Here are some of the incredibly important skills we learned that will benefit us hugely 
in industry:
- Working with production level software like Strategy Studio
- Working with real market PCAPs from established exchanges
- Significantly enhanced C++ skills
- How to utilize ChatGPT effectively for software development projects
- Basic trading algorithm development and thorough strategy research/mathematics

## Future Improvements

For our index arbitrage strategies, there are definitely more improvements that could be made. Firstly, as mentioned in our
analysis, we think it would be beneficial to focus development on the SPY index arb as opposed to working on DIA, as SPY is market_cap weighted
weighted, gives us better chances of accurately predicting price changes. One clear optimization is to parse more holdings of
SPY which will again give us an even more representative R_under. To make more mathematical optimizations, I think we would
definitely have to do much more mathematical research, due to our limited math background we do not have a large amount of more
mathematical optimizations.