/*================================================================================
*     Source: StrategyStudio example templates
*     Contents: SPY-QQQ Direct Arbitrage Strategy (ETF-ETF spread z-score)
*================================================================================*/

#pragma once

#ifndef _SPY_QQQ_DIRECT_ARB_STRATEGY_H_
#define _SPY_QQQ_DIRECT_ARB_STRATEGY_H_

#ifdef _WIN32
    #define _STRATEGY_EXPORTS __declspec(dllexport)
#else
    #ifndef _STRATEGY_EXPORTS
    #define _STRATEGY_EXPORTS
    #endif
#endif

#include <Strategy.h>
#include <Analytics/ScalarRollingWindow.h>
#include <MarketModels/Instrument.h>
#include <Utilities/ParseConfig.h>

#include <boost/unordered_map.hpp>
#include <string>
#include <iostream>
#include <cmath>
#include <cstring>

using namespace RCM::StrategyStudio;
using namespace RCM::StrategyStudio::MarketModels;

enum TradeDirection {
    TRADE_BUY,
    TRADE_SELL
};

class SPYQQQDirectArbStrategy : public Strategy {
public:
    SPYQQQDirectArbStrategy(StrategyID strategyID, const std::string& strategyName, const std::string& groupName);
    ~SPYQQQDirectArbStrategy();

public: /* from IEventCallback */
    virtual void OnTrade(const TradeDataEventMsg& msg);
    virtual void OnTopQuote(const QuoteEventMsg& msg);
    virtual void OnQuote(const QuoteEventMsg& msg);
    virtual void OnDepth(const MarketDepthEventMsg& msg);
    virtual void OnBar(const BarEventMsg& msg);
    virtual void OnMarketState(const MarketStateEventMsg& msg) {};
    virtual void OnOrderUpdate(const OrderUpdateEventMsg& msg);
    virtual void OnStrategyControl(const StrategyStateControlEventMsg& msg) {}

    void OnResetStrategyState();
    void OnDataSubscription(const DataSubscriptionEventMsg& msg) {}
    void OnStrategyCommand(const StrategyCommandEventMsg& msg);
    void OnParamChanged(StrategyParam& param);

private: /* from Strategy */
    virtual void RegisterForStrategyEvents(StrategyEventRegister* eventRegister, DateType currDate);
    virtual void DefineStrategyParams();
    virtual void DefineStrategyCommands();

private: // Helpers
    void ExecuteTrade(double z);
    void SendOrder(const Instrument* instrument, int quantity, TradeDirection direction);
    double ComputeMicroPrice(const Instrument& instrument) const;

private:
    // Instruments
    const Instrument* spy_instrument_;
    const Instrument* qqq_instrument_;

    // Latest microprices
    boost::unordered_map<std::string, double> last_price_;

    // Rolling beta and spread
    Analytics::ScalarRollingWindow<double> beta_window_;   // mean(SPY/QQQ)
    Analytics::ScalarRollingWindow<double> spread_window_; // z-score on spread

    // Params
    double z_score_threshold_;
    double max_z_;
    int position_size_;
    bool debug_;
};

extern "C" {

    _STRATEGY_EXPORTS const char* GetType()
    {
        return "SPYQQQDirectArbStrategy";
    }

    _STRATEGY_EXPORTS IStrategy* CreateStrategy(const char* strategyType,
                                   unsigned strategyID,
                                   const char* strategyName,
                                   const char* groupName)
    {
        if (strcmp(strategyType, GetType()) == 0) {
            return *(new SPYQQQDirectArbStrategy(strategyID, strategyName, groupName));
        } else {
            return NULL;
        }
    }

    // must match an existing user within the system
    _STRATEGY_EXPORTS const char* GetAuthor()
    {
        return "dlariviere";
    }

    // must match an existing trading group within the system
    _STRATEGY_EXPORTS const char* GetAuthorGroup()
    {
        return "UIUC";
    }

    _STRATEGY_EXPORTS const char* GetReleaseVersion()
    {
        return Strategy::release_version();
    }
}

#endif