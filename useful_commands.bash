# List number of text tick data files for each ticker
ls /mnt/2tbnvme/text_tick_data/tick_* | sed -E 's#.*/tick_(.+?)_.+.#\1#' | sort | uniq -c | sort -k2

# Soft link local text_tick_data to the directory with our text_tick_data
unlink ~/ss/bt/text_tick_data
ln -s /mnt/2tbnvme/text_tick_data/ ~/ss/bt/text_tick_data/
ln -s /shared/ss/bt/text_tick_data/ ~/ss/bt/text_tick_data/
ln -s <text_tick_file> ~/ss/bt/text_tick_data/

# Run backtest
./run_backtest.sh --start-date 2019-09-13 --end-date 2019-09-13
./run_backtest.sh --start-date 2023-01-03 --end-date 2023-01-03
./run_backtest.sh --start-date 2022-08-01 --end-date 2023-08-01