#!/bin/bash

# Function to show usage and exit
usage() {
    echo "Usage: $0 --start-date YYYY-MM-DD --end-date YYYY-MM-DD"
    exit 1
}

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --start-date)
            startDate="$2"
            shift 2
            ;;
        --end-date)
            endDate="$2"
            shift 2
            ;;
        *)
            echo "Unknown parameter: $1"
            usage
            ;;
    esac
done

# Validate that both arguments are provided
if [[ -z "$startDate" || -z "$endDate" ]]; then
    echo "Error: both --start-date and --end-date are required."
    usage
fi

# Validate date format (YYYY-MM-DD)
if ! [[ "$startDate" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ && "$endDate" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]; then
    echo "Error: Dates must be in YYYY-MM-DD format."
    usage
fi

logFileNumLines=$(wc -l < "$HOME/ss/bt/logs/main_log.txt")

instanceName='TestOneSPYQQQDirectArbStrategy'

(cd "$HOME/ss/bt" && ./StrategyServerBacktesting &)

echo "Sleeping for 2 seconds while waiting for strategy studio to boot"
sleep 2

# Start the backtest
(cd "$HOME/ss/bt/utilities" && ./StrategyCommandLine cmd start_backtest "$startDate" "$endDate" "$instanceName" 0)

# Get the line number which ends with finished.
foundFinishedLogFile=$(grep -nr "finished.$" "$HOME/ss/bt/logs/main_log.txt" | gawk '{print $1}' FS=":" | tail -1)

# DEBUGGING OUTPUT
echo "Last line found: $foundFinishedLogFile"

# Wait for strategy to finish
while ((logFileNumLines >= foundFinishedLogFile)); do
    echo "Waiting for strategy to finish"
    sleep 10
    foundFinishedLogFile=$(grep -nr "finished.$" "$HOME/ss/bt/logs/main_log.txt" | gawk '{print $1}' FS=":" | tail -1)
    echo "Last line found: $foundFinishedLogFile"
done

echo "Sleeping for 2 seconds..."
sleep 2

# Get the latest file in terms of modified time
latestCRA=$(ls -t "$HOME/ss/bt/backtesting-results" | head -1)

#DEBUGGING OUTPUT
echo "latest CRA File is: $latestCRA"

# Run the export command to generate csv files
cd "$HOME/ss/bt/utilities"
./StrategyCommandLine cmd export_cra_file backtesting-results/$latestCRA backtesting-cra-exports
./StrategyCommandLine cmd quit

# print PnL
cd "$HOME/ss/bt/backtesting-cra-exports"
latestPnL=$(ls -t *pnl* | head -1)
tail -1 "$latestPnL"

VENV_PY="$HOME/ss/sdk/RCM/StrategyStudio/group_03_project/.venv/bin/python3"
"$VENV_PY" "$HOME/ss/sdk/RCM/StrategyStudio/group_03_project/plot_pnl.py" "$latestPnL"

# Generate portfolio metric
# cd /home/vagrant/Desktop/strategy_studio/backtesting/utilities
# python /home/vagrant/Desktop/strategy_studio/localdev/RCM/StrategyStudio/examples/fin566_fall_2020_group_three/metrics/calc_metric.py 

echo "run_backtest.sh: Strategy Studio finished backtesting"

