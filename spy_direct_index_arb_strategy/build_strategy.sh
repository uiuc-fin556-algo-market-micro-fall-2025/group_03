#!/bin/bash

# Base project path
PROJECT_DIR="$HOME/ss/sdk/RCM/StrategyStudio/group_03_project"
# Strategy folder name
STRATEGY_NAME="spy_direct_index_arb_strategy"
# Full strategy path
STRATEGY_DIR="$PROJECT_DIR/$STRATEGY_NAME"
# Instance name
INSTANCE_NAME="TestOneSPYQQQDirectArbStrategy"
# Strategy type (must match GetType() in the .h)
STRATEGY_TYPE="SPYQQQDirectArbStrategy"
# Symbols for this strategy
SYMBOLS="SPY|QQQ"

cd "$PROJECT_DIR"

# Remove hard coded references in the checked out project to /home/vagrant
sed -i.bak "s|/home/vagrant|~|g" "$STRATEGY_DIR/Makefile"

cd "$STRATEGY_DIR"

# Ensure dll folder exists (if you have permission in your ~/ss)
mkdir -p "$HOME/ss/bt/strategies_dlls"

# Build + copy the .so
make clean
make copy_strategy

# Start backtester
cd "$HOME/ss/bt/"
./StrategyServerBacktesting &

sleep 2

# Create instance
cd "$HOME/ss/bt/utilities"
./StrategyCommandLine cmd terminate "$INSTANCE_NAME" || true
./StrategyCommandLine cmd create_instance "$INSTANCE_NAME" "$STRATEGY_TYPE" \
    UIUC SIM-1001-101 dlariviere 1000000 -symbols "$SYMBOLS"
./StrategyCommandLine cmd strategy_instance_list
./StrategyCommandLine cmd quit