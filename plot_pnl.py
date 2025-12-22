# #!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt
import sys

def main():
    if len(sys.argv) < 2:
        print("Usage: python plot_pnl.py <pnl_csv_file>")
        sys.exit(1)

    pnl_file = sys.argv[1]

    # Read CSV
    df = pd.read_csv(pnl_file)

    # Parse timestamps (mixed formats allowed)
    df["Time"] = pd.to_datetime(df["Time"], format="mixed")

    # Sort to guarantee correct order
    df = df.sort_values("Time").reset_index(drop=True)

    # Convert time to string labels (date-level is enough)
    df["TimeStr"] = df["Time"].dt.strftime("%Y-%m-%d")

    # Numeric x-axis: one unit per row (equal spacing)
    x = range(len(df))

    # Plot
    plt.figure(figsize=(14, 6))
    plt.plot(x, df["Cumulative PnL"], linewidth=1)

    # ---- Axis labeling logic ----
    # Show ~8–10 x-axis labels total (prevents clutter)
    num_ticks = 8
    step = max(1, len(df) // num_ticks)

    tick_positions = list(range(0, len(df), step))
    tick_labels = df["TimeStr"].iloc[::step]

    plt.xticks(tick_positions, tick_labels, rotation=45, ha="right")

    # Labels & title
    plt.xlabel("Date")
    plt.ylabel("Cumulative PnL")
    plt.title("Cumulative PnL Over Time (overnight gaps removed)")

    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    # Save output
    plt.savefig(
        "/home/ellisi2/ss/sdk/RCM/StrategyStudio/group_03_project/pnl.png",
        dpi=150
    )

if __name__ == "__main__":
    main()
