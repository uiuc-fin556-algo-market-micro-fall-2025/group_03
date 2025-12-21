/*================================================================================
*     Source: StrategyStudio example templates
*     Contents: SPY-QQQ Direct Arbitrage Strategy (ETF-ETF spread z-score)
*================================================================================*/

#ifdef _WIN32
    #include "stdafx.h"
#endif

#include "SPYQQQDirectArb.h"

#include "FillInfo.h"
#include "AllEventMsg.h"
#include "ExecutionTypes.h"
#include <Utilities/Cast.h>
#include <Utilities/utils.h>

#include <math.h>
#include <iostream>
#include <cassert>

using namespace RCM::StrategyStudio;
using namespace RCM::StrategyStudio::MarketModels;
using namespace RCM::StrategyStudio::Utilities;

using namespace std;

SPYQQQDirectArbStrategy::SPYQQQDirectArbStrategy(StrategyID strategyID,
                                                 const std::string& strategyName,
                                                 const std::string& groupName):
    Strategy(strategyID, strategyName, groupName),
    spy_instrument_(nullptr),
    qqq_instrument_(nullptr),
    beta_window_(50),            // rolling beta window (mean SPY/QQQ)
    spread_window_(50),          // rolling spread window for Z-score
    z_score_threshold_(3.0),     // start conservative 
    max_z_(6.0),
    position_size_(100),
    debug_(false)
{
}

SPYQQQDirectArbStrategy::~SPYQQQDirectArbStrategy()
{
}

void SPYQQQDirectArbStrategy::OnResetStrategyState()
{
    last_price_.clear();
    beta_window_.clear();
    spread_window_.clear();
}

void SPYQQQDirectArbStrategy::DefineStrategyParams()
{
}

void SPYQQQDirectArbStrategy::DefineStrategyCommands()
{
}

void SPYQQQDirectArbStrategy::RegisterForStrategyEvents(StrategyEventRegister* eventRegister, DateType currDate)
{
    for (SymbolSetConstIter it = symbols_begin(); it != symbols_end(); ++it) {
        EventInstrumentPair p = eventRegister->RegisterForBars(*it, BAR_TYPE_TIME, 1);

        const std::string& sym = p.second->symbol();

        if (sym == "SPY") {
            spy_instrument_ = p.second;
        } else if (sym == "QQQ") {
            qqq_instrument_ = p.second;
        }
    }
}

void SPYQQQDirectArbStrategy::OnTrade(const TradeDataEventMsg& msg)
{
    // Not used (we use micro-price from TopQuote), but left here for completeness.
}

double SPYQQQDirectArbStrategy::ComputeMicroPrice(const Instrument& instrument) const
{
    const Quote& tq = instrument.top_quote();

    double bid = tq.bid();
    double ask = tq.ask();
    int bid_sz = tq.bid_size();
    int ask_sz = tq.ask_size();

    // Microprice: size-weighted mid
    if (bid > 0 && ask > 0 && bid_sz > 0 && ask_sz > 0)
        return (bid * ask_sz + ask * bid_sz) / static_cast<double>(bid_sz + ask_sz);

    // Fallbacks
    if (bid > 0 && ask > 0)
        return 0.5 * (bid + ask);

    if (instrument.last_trade().price() > 0)
        return instrument.last_trade().price();

    return 0.0;
}

void SPYQQQDirectArbStrategy::OnTopQuote(const QuoteEventMsg& msg)
{
    const std::string& sym = msg.instrument().symbol();

    // Only care about SPY and QQQ
    if (sym != "SPY" && sym != "QQQ")
        return;

    double micro_price = ComputeMicroPrice(msg.instrument());
    if (micro_price <= 0.0)
        return;

    last_price_[sym] = micro_price;
}

void SPYQQQDirectArbStrategy::OnQuote(const QuoteEventMsg& msg)
{
}

void SPYQQQDirectArbStrategy::OnDepth(const MarketDepthEventMsg& msg)
{
}

void SPYQQQDirectArbStrategy::OnBar(const BarEventMsg& msg)
{
    if (!spy_instrument_ || !qqq_instrument_)
        return;

    // Need both microprices
    if (!last_price_.count("SPY") || !last_price_.count("QQQ"))
        return;

    double p_spy = last_price_["SPY"];
    double p_qqq = last_price_["QQQ"];

    if (p_spy <= 0.0 || p_qqq <= 0.0)
        return;

    // Update rolling beta ~= mean(SPY/QQQ)
    double beta = p_spy / p_qqq;
    beta_window_.push_back(beta);

    if (!beta_window_.full())
        return;

    double beta_hat = beta_window_.Mean();

    // Spread (direct arb): SPY - beta_hat * QQQ
    double spread = p_spy - beta_hat * p_qqq;

    spread_window_.push_back(spread);

    if (!spread_window_.full())
        return;

    if (orders().num_working_orders() > 0)
        return;

    double z = spread_window_.ZScore();

    if (debug_) {
        std::stringstream ss;
        ss << "p_spy=" << p_spy
           << " p_qqq=" << p_qqq
           << " beta_hat=" << beta_hat
           << " spread=" << spread
           << " z=" << z;
        logger().LogToClient(LOGLEVEL_DEBUG, ss.str());
    }

    ExecuteTrade(z);
}

void SPYQQQDirectArbStrategy::ExecuteTrade(double z)
{
    // We trade SPY only (simple execution); signal is relative pricing vs QQQ.
    int target_position = 0;
    double abs_z = std::fabs(z);

    // No-trade zone
    if (abs_z <= z_score_threshold_) {
        target_position = 0;
    } else {
        // Scale between threshold and max_z
        double scale =
            (abs_z - z_score_threshold_) /
            (max_z_ - z_score_threshold_);

        if (scale > 1.0)
            scale = 1.0;

        int sized_position =
            static_cast<int>(std::round(scale * position_size_));

        // If spread is positive (z>0): SPY rich -> SELL SPY
        // If spread is negative (z<0): SPY cheap -> BUY SPY
        target_position = (z > 0) ? -sized_position : sized_position;
    }

    int current_pos = portfolio().position(spy_instrument_);
    int size_needed = target_position - current_pos;

    if (size_needed > 0) {
        SendOrder(spy_instrument_, size_needed, TRADE_BUY);
    } else if (size_needed < 0) {
        SendOrder(spy_instrument_, -size_needed, TRADE_SELL);
    }
}

void SPYQQQDirectArbStrategy::SendOrder(
    const Instrument* instrument,
    int quantity,
    TradeDirection direction)
{
    OrderSide side =
        (direction == TRADE_BUY) ? ORDER_SIDE_BUY : ORDER_SIDE_SELL;

    double price = 0.0;

    if (direction == TRADE_BUY) {
        price = (instrument->top_quote().ask() != 0)
            ? instrument->top_quote().ask()
            : instrument->last_trade().price();
    } else {
        price = (instrument->top_quote().bid() != 0)
            ? instrument->top_quote().bid()
            : instrument->last_trade().price();
    }

    OrderParams params(
        *instrument,
        quantity,
        price,
        MARKET_CENTER_ID_IEX,
        side,
        ORDER_TIF_DAY,
        ORDER_TYPE_LIMIT
    );

    trade_actions()->SendNewOrder(params);
}

void SPYQQQDirectArbStrategy::OnOrderUpdate(const OrderUpdateEventMsg& msg)
{
}

void SPYQQQDirectArbStrategy::OnStrategyCommand(const StrategyCommandEventMsg& msg)
{
}

void SPYQQQDirectArbStrategy::OnParamChanged(StrategyParam& param)
{
}