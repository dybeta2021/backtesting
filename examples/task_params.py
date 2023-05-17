#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Project  : backtesting 
# @File     : task_params.py
# @Author   : Gavin
# @Time     : 2023/5/17 10:57
# @Desc     :
import sqlite3
import subprocess

from pandas import read_pickle, concat, read_sql

underlying_symbol = "RB"
start_date = "2021-01-01"
end_date = "2023-05-15"
raw_data = read_pickle("access/future_weight_index_60min.pkl", compression='gzip')
data = raw_data.loc[raw_data["symbol"] == "{}".format(underlying_symbol)].dropna()

conn = sqlite3.connect("FullUri=file:mydb.sqlite?cache=shared")
data.to_sql('quote', conn, if_exists="replace")

child = subprocess.Popen(['./ama_params', 'FullUri=file:mydb.sqlite?cache=shared', '2020-01-01', '2023-05-12'],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
while True:
    line = child.stdout.readline()
    if not line:
        break
#     print(line.decode('utf-8'))


conn = sqlite3.connect("result.sqlite")
df = read_sql("SELECT * FROM params", conn)
df = df.sort_values("final_pnl", ascending=False)
del df["ama_long"]
df1 = df.head(100)
df2 = df.tail(100)
print(df1)
