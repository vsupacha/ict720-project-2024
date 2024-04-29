import pandas as pd
import streamlit as st

# config
[start_year,end_year] = st.sidebar.slider('Start Year', 2018, 2023, [2018,2023])
start_year = start_year - 2000
end_year = end_year - 2000
month_dict = {"Jan":"01", "Feb":"02", "Mar":"03", "Apr":"04", "May":"05", "Jun":"06", "Jul":"07", "Aug":"08", "Sep":"09", "Oct":"10", "Nov":"11", "Dec":"12"}
month_name = st.sidebar.selectbox('Month', list(month_dict.keys()))
month = month_dict[month_name]

# main
years = [str(year) for year in range(start_year,end_year+1)]
tabs = st.tabs( years )
year_idx = 0
for year in years:
    url = f'http://peaoc.pea.co.th/loadprofile/files/07/dt07{year}{month}30.xls'
    idx = f'{year}-{month}'
    if idx not in st.session_state:
        df = pd.read_excel(url, sheet_name='Source', header=4)
        tmp_df = df.copy()
        tmp_df.iloc[0:96, 1:6] = tmp_df.iloc[1:97, 1:6] # move one row up
        tmp_df.drop(index=96, inplace=True) # drop last row
        if (tmp_df.WORKDAY.iloc[-1] < tmp_df.WORKDAY.iloc[0]) & (tmp_df.WORKDAY.iloc[-1] < tmp_df.WORKDAY.iloc[-2]):
            print('bad data')
            tmp_df.iloc[95,1:6] = tmp_df.iloc[94,1:6] # substitue with data before
        st.session_state[idx] = tmp_df
    else:
        tmp_df = st.session_state[idx]
    with tabs[year_idx]:
        new_tmp_df = st.data_editor(tmp_df)
        st.line_chart(new_tmp_df.WORKDAY)
    year_idx += 1