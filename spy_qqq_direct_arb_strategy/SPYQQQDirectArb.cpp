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
#include <algorithm>

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
    debug_(false),

    // hysteresis defaults
    z_enter_(3.0),     // keep same as old threshold
    z_exit_(1.2),      // exit earlier to avoid churn
    in_trade_(false),

    // microstructure defaults
    max_spread_bps_(2.0),
    min_top_size_(100),
    use_passive_exec_(true),
    price_improve_ticks_(1),
    cooldown_bars_(2),
    bars_since_trade_(1000000)
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
    bars_since_trade_ = 1000000;

    // NEW
    in_trade_ = false;
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

    if (bid > 0 && ask > 0 && bid_sz > 0 && ask_sz > 0)
        return (bid * ask_sz + ask * bid_sz) / static_cast<double>(bid_sz + ask_sz);

    if (bid > 0 && ask > 0)
        return 0.5 * (bid + ask);

    if (instrument.last_trade().price() > 0)
        return instrument.last_trade().price();

    return 0.0;
}

void SPYQQQDirectArbStrategy::OnTopQuote(const QuoteEventMsg& msg)
{
    const std::string& sym = msg.instrument().symbol();

    if (sym != "SPY" && sym != "QQQ")
        return;

    double micro_price = ComputeMicroPrice(msg.instrument());
    if (micro_price <= 0.0)
        return;

    last_price_[sym] = micro_price;
}

void SPYQQQDirectArbStrategy::OnQuote(const QuoteEventMsg& msg) {}
void SPYQQQDirectArbStrategy::OnDepth(const MarketDepthEventMsg& msg) {}

bool SPYQQQDirectArbStrategy::MarketQualityOk() const
{
    if (!spy_instrument_ || !qqq_instrument_)
        return false;

    const Quote& s = spy_instrument_->top_quote();
    const Quote& q = qqq_instrument_->top_quote();

    auto ok_one = [&](const Quote& tq) -> bool {
        double bid = tq.bid();
        double ask = tq.ask();
        int bsz = tq.bid_size();
        int asz = tq.ask_size();

        if (bid <= 0.0 || ask <= 0.0 || ask <= bid)
            return false;

        if (bsz < min_top_size_ || asz < min_top_size_)
            return false;

        double mid = 0.5 * (bid + ask);
        double spr = ask - bid;
        double spr_bps = (spr / mid) * 10000.0;

        return spr_bps <= max_spread_bps_;
    };

    return ok_one(s) && ok_one(q);
}

double SPYQQQDirectArbStrategy::GetTickSize(const Instrument* instrument) const
{
    (void)instrument;
    return 0.01;
}

void SPYQQQDirectArbStrategy::OnBar(const BarEventMsg& msg)
{
    if (!spy_instrument_ || !qqq_instrument_)
        return;

    bars_since_trade_++;

    if (!last_price_.count("SPY") || !last_price_.count("QQQ"))
        return;

    double p_spy = last_price_["SPY"];
    double p_qqq = last_price_["QQQ"];

    if (p_spy <= 0.0 || p_qqq <= 0.0)
        return;

    double beta = p_spy / p_qqq;
    beta_window_.push_back(beta);

    if (!beta_window_.full())
        return;

    double beta_hat = beta_window_.Mean();

    double spread = p_spy - beta_hat * p_qqq;
    spread_window_.push_back(spread);

    if (!spread_window_.full())
        return;

    if (orders().num_working_orders() > 0)
        return;

    if (!MarketQualityOk())
        return;

    if (bars_since_trade_ < cooldown_bars_)
        return;

    double z = spread_window_.ZScore();

    if (debug_) {
        std::stringstream ss;
        ss << "p_spy=" << p_spy
           << " p_qqq=" << p_qqq
           << " beta_hat=" << beta_hat
           << " spread=" << spread
           << " z=" << z
           << " in_trade=" << (in_trade_ ? "1" : "0");
        logger().LogToClient(LOGLEVEL_DEBUG, ss.str());
    }

    ExecuteTrade(z);
}

void SPYQQQDirectArbStrategy::ExecuteTrade(double z)
{
    double abs_z = std::fabs(z);

    // --- hysteresis gate ---
    if (!in_trade_) {
        // only enter when signal is strong
        if (abs_z < z_enter_) {
            return;
        }
        in_trade_ = true;
    } else {
        // only exit when signal meaningfully decays
        if (abs_z <= z_exit_) {
            int current_pos = portfolio().position(spy_instrument_);
            if (current_pos > 0) {
                SendOrder(spy_instrument_, current_pos, TRADE_SELL);
            } else if (current_pos < 0) {
                SendOrder(spy_instrument_, -current_pos, TRADE_BUY);
            }
            in_trade_ = false;
            return;
        }
    }

    // --- sizing logic ---
    int target_position = 0;

    double scale =
        (abs_z - z_enter_) /
        (max_z_ - z_enter_);

    if (scale > 1.0) scale = 1.0;

    int sized_position =
        static_cast<int>(std::round(scale * position_size_));

    target_position = (z > 0) ? -sized_position : sized_position;

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

    const Quote& tq = instrument->top_quote();
    double bid = tq.bid();
    double ask = tq.ask();

    double price = 0.0;
    double tick = GetTickSize(instrument);

    if (!use_passive_exec_) {
        if (direction == TRADE_BUY) {
            price = (ask != 0) ? ask : instrument->last_trade().price();
        } else {
            price = (bid != 0) ? bid : instrument->last_trade().price();
        }
    } else {
        if (bid > 0.0 && ask > 0.0 && ask > bid) {
            if (direction == TRADE_BUY) {
                double px = bid + price_improve_ticks_ * tick;
                price = std::min(px, ask);
            } else {
                double px = ask - price_improve_ticks_ * tick;
                price = std::max(px, bid);
            }
        } else {
            price = (instrument->last_trade().price() > 0.0)
                ? instrument->last_trade().price()
                : 0.0;
        }
    }

    if (price <= 0.0)
        return;

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

    bars_since_trade_ = 0;
}

void SPYQQQDirectArbStrategy::OnOrderUpdate(const OrderUpdateEventMsg& msg) {}
void SPYQQQDirectArbStrategy::OnStrategyCommand(const StrategyCommandEventMsg& msg) {}
void SPYQQQDirectArbStrategy::OnParamChanged(StrategyParam& param) {}