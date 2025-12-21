#!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import sys
from datetime import timedelta

def main():
    if len(sys.argv) < 2:
        print("Usage: python plot_pnl.py <pnl_csv_file>")
        sys.exit(1)

    pnl_file = sys.argv[1]

    # Read CSV
    df = pd.read_csv(pnl_file)

    # Convert Time column to datetime
    df["Time"] = pd.to_datetime(df["Time"], format="mixed")

    # Sort by time
    df = df.sort_values("Time").reset_index(drop=True)

    # Compress overnight gaps
    compressed_times = []
    last_time = df["Time"].iloc[0]
    compressed_times.append(last_time)

    for current_time in df["Time"].iloc[1:]:
        diff = current_time - last_time
        # If the gap is more than 4 hours (overnight), compress to 1 minute
        if diff > timedelta(hours=4):
            new_time = compressed_times[-1] + timedelta(minutes=1)
        else:
            new_time = compressed_times[-1] + diff
        compressed_times.append(new_time)
        last_time = current_time

    df["CompressedTime"] = compressed_times

    # Plot
    plt.figure(figsize=(14, 6))
    plt.plot(df["CompressedTime"], df["Cumulative PnL"], marker='o', markersize=2, linestyle='-')

    # Format x-axis to show actual dates nicely
    plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d %H:%M'))
    plt.gcf().autofmt_xdate(rotation=45)
    plt.xlabel("Time (overnight gaps compressed)")
    plt.ylabel("Cumulative PnL")
    plt.title("Cumulative PnL Over Time")
    plt.grid(True)
    plt.tight_layout()

    plt.savefig("/home/ellisi2/ss/sdk/RCM/StrategyStudio/group_03_project/pnl.png")

if __name__ == "__main__":
    main()
