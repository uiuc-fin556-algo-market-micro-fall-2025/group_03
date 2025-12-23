# group_03_project

Our report is located in report.md

Within the README, we outline how to set up the strategy studio environment to run our strategies

## Setup to run our strategies

### StrategyStudio

The ss directory must be cloned from whatever location it is stored at

Once you have the ss directory, git clone this project into ss/sdk/RCM/StrategyStudio/

This should be sufficient to build and execute the strategy studio strategies

### Python

In order to run our plotting scripts to visualize PnL, you must create a venv in the project directory

Once created, activate the venv and install the following dependencies

```bash
python -m pip install pandas
python -m pip install matplotlib
python -m pip install datetime
```

Additionally, the VENV_PY variable in the run_backtest.sh script within each strategy directory must be updated to the path of
your python interpreter within the VENV_PY

Within the plotting script, the plt.savefig should be updated to save the figure to the desired location

## Run a strategy

To run a strategy, cd into one of the strategy directories and run the following

```bash
make
./build_strategy.sh
./run_backtest.sh --start-date YYYY-MM-DD --end-date YYYY-MM-DD
```

This will build the object files for the strategy, initialize the strategy within strategy studio, run the strategy
across the desired dates, as well as process the results into a CSV and plot the PnL results.

## Results 

Raw PnL and order data can be seen in the processed CSV files written to ss/bt/backtesting-cra-results/

The PnL plot can also be viewed at whatever file location you specified in the PnL script