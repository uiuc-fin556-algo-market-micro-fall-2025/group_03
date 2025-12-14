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
    spy_instrument(nullptr)
{
}

SpyIndexArbStrategy::~SpyIndexArbStrategy()
{
}

void SpyIndexArbStrategy::OnResetStrategyState()
{
}

void SpyIndexArbStrategy::DefineStrategyParams()
{
}

void SpyIndexArbStrategy::DefineStrategyCommands()
{
}

void SpyIndexArbStrategy::RegisterForStrategyEvents(StrategyEventRegister* eventRegister, DateType currDate)
{    
    for (InstrumentSetConstIter it = instrument_begin(); it != instrument_end(); ++it) {
        if (it->second->symbol().compare("SPY")==0) {
            spy_instrument = it->second;
        }
    }
}
void SpyIndexArbStrategy::OnTrade(const TradeDataEventMsg& msg)
{
    // msg.instrument() is const, so grab its ID
    const Instrument* instrument = &msg.instrument();
    if (instrument->symbol().compare("SPY")==0) {
        return;
    }

    int quantity = 1;   // any quantity you want

    // Take price just better than than current top of book
    double px = msg.side() == TRADE_SIDE_BUY ? instrument->top_quote().bid()+0.01 : instrument->top_quote().ask()-0.01;

    // Pick a market center depending on instrument type
    MarketCenterID mcid = MARKET_CENTER_ID_IEX;

    // Take same side as trade
    OrderSide side = msg.side() == TRADE_SIDE_BUY ? ORDER_SIDE_BUY : ORDER_SIDE_SELL;

    OrderParams params(
        *spy_instrument,
        quantity,
        px,
        mcid,
        side,
        ORDER_TIF_DAY,
        ORDER_TYPE_LIMIT
    );

    trade_actions()->SendNewOrder(params);

	// std::cout << "OnTrade(): (" << msg.adapter_time() << "): " << msg.instrument().symbol() << ": " << msg.trade().size() << " @ $" << msg.trade().price() << '\n';
	/*
	std::cout << "\t " <<
				msg.instrument().top_quote().bid_size() << " @ $"<< msg.instrument().top_quote().bid() <<
				msg.instrument().top_quote().ask_size() << " @ $"<< msg.instrument().top_quote().ask() <<
				std::endl;
	*/
}

void SpyIndexArbStrategy::OnTopQuote(const QuoteEventMsg& msg)
{
	// std::cout << "OnTopQuote(): (" << msg.adapter_time() << "): " << msg.instrument().symbol() << ": " <<
	// 	msg.instrument().top_quote().bid_size() << " @ $"<< msg.instrument().top_quote().bid() << ", " <<
	// 	msg.instrument().top_quote().ask_size() << " @ $"<< msg.instrument().top_quote().ask() <<
	// 	'\n';

}

void SpyIndexArbStrategy::OnQuote(const QuoteEventMsg& msg)
{
	// std::cout << "OnQuote(): (" << msg.adapter_time() << "): " << msg.instrument().symbol() << ": " <<
    //     msg.instrument().top_quote().bid_size() << " @ $"<< msg.instrument().top_quote().bid() << ", " <<
    //     msg.instrument().top_quote().ask_size() << " @ $"<< msg.instrument().top_quote().ask() <<
    //     '\n';
}

void SpyIndexArbStrategy::OnDepth(const MarketDepthEventMsg& msg)
{
	// std::cout << "OnDepth(): (" << msg.adapter_time() << "): " << msg.instrument().symbol() << ": " <<
    //     msg.instrument().top_quote().bid_size() << " @ $"<< msg.instrument().top_quote().bid() << ", " <<
    //     msg.instrument().top_quote().ask_size() << " @ $"<< msg.instrument().top_quote().ask() <<
    //     '\n';
}

void SpyIndexArbStrategy::OnBar(const BarEventMsg& msg)
{
}

void SpyIndexArbStrategy::OnOrderUpdate(const OrderUpdateEventMsg& msg)
{    
	// std::cout << "OnOrderUpdate(): " << msg.update_time() << msg.name() << std::endl;
}



void SpyIndexArbStrategy::OnStrategyCommand(const StrategyCommandEventMsg& msg)
{
}

void SpyIndexArbStrategy::OnParamChanged(StrategyParam& param)
{    
}
