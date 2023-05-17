#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Project  : examples 
# @File     : task_plot.py 
# @Author   : yangdong
# @Time     : 2023/5/17 22:58
# @Desc     :


import sqlite3
import subprocess

from matplotlib import pyplot
from pandas import read_pickle, read_sql, concat

underlying_symbol = "RB"
start_date = "2021-01-01"
end_date = "2023-05-15"
raw_data = read_pickle("access/future_weight_index_60min.pkl", compression='gzip')
data = raw_data.loc[raw_data["symbol"] == "{}".format(underlying_symbol)].dropna()

conn = sqlite3.connect("FullUri=file:mydb.sqlite?cache=shared")
data.to_sql('quote', conn, if_exists="replace")

child = subprocess.Popen(
    ['./ama_value', 'FullUri=file:mydb.sqlite?cache=shared', start_date, end_date, '2', '30', '11', '3'],
    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
while True:
    line = child.stdout.readline()
    if not line:
        break
    print(line.decode('utf-8'))

conn = sqlite3.connect("value_result.sqlite")
position_df = read_sql("SELECT * FROM position_table", conn)
signal_df = read_sql("SELECT * FROM signal_table", conn)
# order_df = read_sql("SELECT * FROM order_table", conn)


df = []
df.append(
    position_df.loc[position_df["status"] == "post_trade"].set_index("datetime").loc[:, ["current_price", "total_pnl"]])
df.append(signal_df.loc[signal_df["datetime"] >= start_date].set_index("datetime").loc[:,
          ["close_price", "signal", "order_volume"]])
df = concat(df, axis=1)
df["total_pnl"].plot(figsize=(12, 6))
pyplot.show()

order_df = df.loc[df["order_volume"] != 0].copy()
order_df["order_pnl"] = order_df["total_pnl"].shift(-1) - order_df["total_pnl"]
order_df.iloc[-1, -1] = position_df["total_pnl"].iloc[-1] - order_df["total_pnl"].iloc[-1]
order_df["order_pnl"].plot(figsize=(12, 6), kind="bar")
pyplot.show()
