/*================================================================================                               
*     Source: ../RCM/StrategyStudio/examples/strategies/SimpleMomentumStrategy/SimpleMomentumStrategy.cpp                                                        
*     Last Update: 2013/6/1 13:55:14                                                                            
*     Contents:                                     
*     Distribution:          
*                                                                                                                
*                                                                                                                
*     Copyright (c) RCM-X, 2011 - 2013.                                                  
*     All rights reserved.                                                                                       
*                                                                                                                
*     This software is part of Licensed material, which is the property of RCM-X ("Company"), 
*     and constitutes Confidential Information of the Company.                                                  
*     Unauthorized use, modification, duplication or distribution is strictly prohibited by Federal law.         
*     No title to or ownership of this software is hereby transferred.                                          
*                                                                                                                
*     The software is provided "as is", and in no event shall the Company or any of its affiliates or successors be liable for any 
*     damages, including any lost profits or other incidental or consequential damages relating to the use of this software.       
*     The Company makes no representations or warranties, express or implied, with regards to this software.                        
/*================================================================================*/   

#ifdef _WIN32
    #include "stdafx.h"
#endif

#include "SpyIndexArb.h"

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

SpyIndexArbStrategy::SpyIndexArbStrategy(StrategyID strategyID, const std::string& strategyName, const std::string& groupName):
    Strategy(strategyID, strategyName, groupName),
    spy_instrument_(nullptr),
    rolling_delta_(50),          // rolling sigma window
    z_score_threshold_(3),
    max_z_(6),
    position_size_(100)
{
    weights_["AAPL"] = 0.0590;
    weights_["MSFT"] = 0.0560;
    weights_["AMZN"] = 0.0405;
    weights_["GOOG"] = 0.0196;
    weights_["NVDA"] = 0.0137;
}

SpyIndexArbStrategy::~SpyIndexArbStrategy()
{
}

void SpyIndexArbStrategy::OnResetStrategyState()
{
    rolling_delta_.clear();
    last_price_.clear();
    ref_price_.clear();
}

void SpyIndexArbStrategy::DefineStrategyParams()
{
}

void SpyIndexArbStrategy::DefineStrategyCommands()
{
}

void SpyIndexArbStrategy::RegisterForStrategyEvents(StrategyEventRegister* eventRegister, DateType currDate)
{    
    for (SymbolSetConstIter it = symbols_begin(); it != symbols_end(); ++it) {
        EventInstrumentPair p = eventRegister->RegisterForBars(*it, BAR_TYPE_TIME, 1);

        const std::string& sym = p.second->symbol();

        if (sym == "SPY") {
            spy_instrument_ = p.second;
        } else if (weights_.count(sym)) {
            underlyings_[sym] = p.second;
        }
    }
}
void SpyIndexArbStrategy::OnTrade(const TradeDataEventMsg& msg)
{
}

double SpyIndexArbStrategy::ComputeMicroPrice(const Instrument& instrument) const
{
    const Quote& tq = instrument.top_quote();

    double bid = tq.bid();
    double ask = tq.ask();
    int bid_sz = tq.bid_size();
    int ask_sz = tq.ask_size();

    if (bid > 0 && ask > 0 && bid_sz > 0 && ask_sz > 0) 
        return (bid * ask_sz + ask * bid_sz) / static_cast<double>(bid_sz + ask_sz);

    // Fallbacks
    if (bid > 0 && ask > 0)
        return 0.5 * (bid + ask);

    if (instrument.last_trade().price() > 0)
        return instrument.last_trade().price();

    return 0.0;
}

void SpyIndexArbStrategy::OnTopQuote(const QuoteEventMsg& msg)
{
    const std::string& sym = msg.instrument().symbol();

    double micro_price = ComputeMicroPrice(msg.instrument());

    if (micro_price <= 0.0)
        return;

    last_price_[sym] = micro_price;

    // Initialize reference price once
    if (!ref_price_.count(sym)) {
        ref_price_[sym] = micro_price;
    }
}

void SpyIndexArbStrategy::OnQuote(const QuoteEventMsg& msg)
{
}

void SpyIndexArbStrategy::OnDepth(const MarketDepthEventMsg& msg)
{
}

void SpyIndexArbStrategy::OnBar(const BarEventMsg& msg)
{
    if (!spy_instrument_) return;

    // Ensure all prices exist
    if (!ref_price_.count("SPY")) return;

    // Compute R_under
    double total_weight = 0.0;
    double r_under = 0.0;
    for (boost::unordered_map<std::string,double>::const_iterator it =
         weights_.begin(); it != weights_.end(); ++it)
    {
        if (ref_price_.count(it->first)) total_weight += it->second;
        else continue;

        const std::string& sym = it->first;
        double w = it->second;

        double r =
            (last_price_[sym] - ref_price_[sym]) / ref_price_[sym];

        r_under += w * r;
    }
    if (total_weight == 0.0) return;

    // Compute R_spy
    double r_spy =
        (last_price_["SPY"] - ref_price_["SPY"]) / ref_price_["SPY"];

    double delta = r_under - total_weight * r_spy;

    // Push delta into rolling window
    rolling_delta_.push_back(delta);

    if (!rolling_delta_.full())
        return;

    if (orders().num_working_orders() > 0)
        return;

    double z = rolling_delta_.ZScore();

    ExecuteTrade(z);
}

void SpyIndexArbStrategy::ExecuteTrade(double z)
{
    int target_position = 0;
    double abs_z = std::fabs(z);

    // No trade zone
    if (abs_z <= z_score_threshold_) {
        target_position = 0;
    } else {
        // Linear scaling between threshold and max_z
        double scale =
            (abs_z - z_score_threshold_) /
            (max_z_ - z_score_threshold_);

        // Cap scale at 1
        if (scale > 1.0)
            scale = 1.0;

        int sized_position =
            static_cast<int>(std::round(scale * position_size_));

        target_position = (z > 0) ? sized_position : -sized_position;
    }

    if (target_position > 0) {
        SendOrder(spy_instrument_, target_position, TRADE_BUY);
    } else if (target_position < 0) {
        SendOrder(spy_instrument_, -target_position, TRADE_SELL);
    }
}

void SpyIndexArbStrategy::SendOrder(
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

void SpyIndexArbStrategy::OnOrderUpdate(const OrderUpdateEventMsg& msg)
{    
}



void SpyIndexArbStrategy::OnStrategyCommand(const StrategyCommandEventMsg& msg)
{
}

void SpyIndexArbStrategy::OnParamChanged(StrategyParam& param)
{    
}
