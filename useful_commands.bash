# List number of text tick data files for each ticker
ls /mnt/2tbnvme/text_tick_data/tick_* | sed -E 's#.*/tick_(.+?)_.+.#\1#' | sort | uniq -c | sort -k2