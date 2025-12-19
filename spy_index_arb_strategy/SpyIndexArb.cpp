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
    z_score_threshold_(1.5),
    position_size_(100),
    debug_(false)
{
    weights_["NVDA"] = 0.0738;
    weights_["MSFT"] = 0.0625;
    weights_["AMZN"] = 0.0387;
    weights_["GOOG"] = 0.0318;
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
    // for (InstrumentSetConstIter it = instrument_begin(); it != instrument_end(); ++it) {
    //     if (it->second->symbol().compare("SPY")==0) {
    //         spy_instrument = it->second;
    //     }
    // }
}
void SpyIndexArbStrategy::OnTrade(const TradeDataEventMsg& msg)
{
    const std::string& sym = msg.instrument().symbol();
    double px = msg.trade().price();

    last_price_[sym] = px;

    if (!ref_price_.count(sym)) {
        ref_price_[sym] = px;
    }
}

void SpyIndexArbStrategy::OnTopQuote(const QuoteEventMsg& msg)
{
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
    for (boost::unordered_map<std::string,double>::const_iterator it = weights_.begin(); it != weights_.end(); ++it)
    {
        if (!ref_price_.count(it->first)) return;
    }

    // Compute R_under
    double r_under = 0.0;
    for (boost::unordered_map<std::string,double>::const_iterator it =
         weights_.begin(); it != weights_.end(); ++it)
    {
        const std::string& sym = it->first;
        double w = it->second;

        double r =
            (last_price_[sym] - ref_price_[sym]) / ref_price_[sym];

        r_under += w * r;
    }

    // Compute R_spy
    double r_spy =
        (last_price_["SPY"] - ref_price_["SPY"]) / ref_price_["SPY"];

    double total_weight = 0.0;
    for (auto& w : weights_) total_weight += w.second;

    double delta = r_under - total_weight * r_spy;

    // Push delta into rolling window
    rolling_delta_.push_back(delta);

    if (!rolling_delta_.full())
        return;

    if (orders().num_working_orders() > 0)
        return;

    double z = rolling_delta_.ZScore();

    if (debug_) {
        std::stringstream ss;
        ss << "Delta=" << delta
           << " Mean=" << rolling_delta_.Mean()
           << " Z=" << z;
        logger().LogToClient(LOGLEVEL_DEBUG, ss.str());
    }

    ExecuteTrade(z);

}

void SpyIndexArbStrategy::ExecuteTrade(double z)
{
    int target_position = 0;

    if (z > z_score_threshold_) {
        target_position = position_size_;
    } else if (z < -z_score_threshold_) {
        target_position = -position_size_;
    }

    int current_pos = portfolio().position(spy_instrument_);
    int size_needed = target_position - current_pos;

    if (size_needed > 0) {
        SendOrder(spy_instrument_, size_needed, TRADE_BUY);
    } else if (size_needed < 0) {
        SendOrder(spy_instrument_, -size_needed, TRADE_SELL);
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

    if (debug_) {
        std::stringstream ss;
        ss << "Sending "
           << ((direction == TRADE_BUY) ? "BUY" : "SELL")
           << " order for " << instrument->symbol()
           << " at price " << price
           << " and quantity " << quantity;
        logger().LogToClient(LOGLEVEL_DEBUG, ss.str());
    }

    OrderParams params(
        *instrument,
        quantity,
        price,
        MARKET_CENTER_ID_IEX,
        side,
        ORDER_TIF_DAY,
        ORDER_TYPE_MARKET
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
