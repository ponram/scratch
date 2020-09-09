import pandas as pd
from datetime import datetime

val_date = datetime.strptime(r'2020/06/11', '%Y/%m/%d')
df_cfg = pd.read_csv('./config.csv')
df_collection = []
for file in df_cfg.File:
    df_temp = pd.read_csv(file, dtype={'TransactionID': str})
    df_collection.append(df_temp)
df_mtm = pd.concat(df_collection)
df_mtm['ScenarioDate'] = pd.to_datetime(df_mtm['ScenarioDate'],
                                        format='%Y/%m/%d')
dates_raw = df_mtm[df_mtm['ScenarioDate'] != val_date]['ScenarioDate']
scen_dates = dates_raw.drop_duplicates()
df_val_date = pd.DataFrame(df_mtm[df_mtm['ScenarioDate'] == val_date])
df_pnl_coll = []
for date in scen_dates:
    df_scen_date = df_mtm[df_mtm['ScenarioDate'] == date]
    df_tmp = pd.merge(df_scen_date, df_val_date, on='TransactionID')
    df_tmp['pnl'] = df_tmp['MTM_x'] - df_tmp['MTM_y']
    df_tmp['ScenarioDate'] = df_tmp['ScenarioDate_x']
    df_pnl_coll.append(df_tmp.reindex(columns=
                                      ['ScenarioDate', 'TransactionID', 'pnl']))
df_pnl = pd.concat(df_pnl_coll)
