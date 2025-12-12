#!/bin/bash

# Check for missing argument
if [[ -z "$1" ]]; then
    echo "Usage: $0 <StrategyName>"
    exit 1
fi
STRAT="$1"

cd "$HOME/ss/bt/"
./StrategyServerBacktesting &

sleep 1

cd "$HOME/ss/bt/utilities"

./StrategyCommandLine cmd terminate "$STRAT"
./StrategyCommandLine cmd strategy_instance_list
./StrategyCommandLine cmd quit