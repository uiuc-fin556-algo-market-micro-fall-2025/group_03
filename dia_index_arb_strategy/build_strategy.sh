#!/bin/bash

# Base project path
PROJECT_DIR="$HOME/ss/sdk/RCM/StrategyStudio/group_03_project"
# Strategy name
STRATEGY_NAME="dia_index_arb_strategy"
# Full strategy path
STRATEGY_DIR="$PROJECT_DIR/$STRATEGY_NAME"
# Instance name
INSTANCE_NAME="TestOneDiaIndexArbStrategy"

cd "$PROJECT_DIR"

# Remove hard coded references in the checked out project to /home/vagrant vs now any user can run it from their own home directory (linux shortcut for your home dir is "~" FYI students)
sed -i.bak "s|/home/vagrant|~|g" "$STRATEGY_DIR/Makefile"

cd "$STRATEGY_DIR"
mkdir -p "$HOME/ss/bt/strategies_dlls"
make copy_strategy

cd "$HOME/ss/bt/"
./StrategyServerBacktesting &

sleep 1

cd "$HOME/ss/bt/utilities"
./StrategyCommandLine cmd terminate "$INSTANCE_NAME"
./StrategyCommandLine cmd create_instance "$INSTANCE_NAME" DiaIndexArbStrategy \
    UIUC SIM-1001-101 dlariviere 1000000 -symbols "DIA|GS|CAT|MSFT|AXP|HD|AAPL|AMZN|NVDA"
./StrategyCommandLine cmd strategy_instance_list
./StrategyCommandLine cmd quit
