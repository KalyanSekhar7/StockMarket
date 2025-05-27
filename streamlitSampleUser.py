import streamlit as st
from kafkaConsumer import get_latest_data
import pandas as pd
import time
import altair as alt

st.set_page_config(page_title="Real-Time Stock Dashboard", layout="wide")
st.title("Stock Market Dashboard")

# Placeholders
placeholder = st.empty()
chart_placeholder = st.empty()
candle_placeholder = st.empty()

# Store price history
price_history = []

while True:
    data = get_latest_data()

    last_price = data["lastTradedPrice"]
    timestamp = data["timestamp"]
    buy_orders = data["buyTable"]
    sell_orders = data["sellTable"]

    # Append to price history
    price_history.append({
        "time": pd.to_datetime(timestamp, unit="ms"),
        "price": last_price
    })
    # Limit history for performance
    # price_history = price_history[-100:]

    df = pd.DataFrame(price_history)

    with placeholder.container():
        st.metric("💰 Last Traded Price", f"${last_price:.2f}")

        col1, col2 = st.columns(2)

        with col1:
            st.subheader("🟢 Buy Orders")
            st.dataframe(buy_orders, height=200, use_container_width=True)
            with st.expander("Show full buy order book"):
                st.dataframe(buy_orders, use_container_width=True)

        with col2:
            st.subheader("🔴 Sell Orders")
            st.dataframe(sell_orders, height=200, use_container_width=True)
            with st.expander("Show full sell order book"):
                st.dataframe(sell_orders, use_container_width=True)


    with chart_placeholder.container():
        st.subheader("📊 Last Traded Price (Line Chart)")
        line_chart = (
            alt.Chart(df)
            .mark_line(point=True)
            .encode(
                x=alt.X('time:T', title='Time', axis=alt.Axis(format='%H:%M:%S', tickCount=10)),
                y=alt.Y('price:Q', title='Price ($)')
            )
            .properties(width=800, height=400)
        )
        st.altair_chart(line_chart, use_container_width=True)

    with candle_placeholder.container():
        st.subheader("🕯️ Candlestick Chart (5s Aggregation)")

        if len(df) >= 2:
            ohlc = (
                df.set_index("time")
                .resample("5S")
                .agg({"price": ["first", "max", "min", "last"]})
                .dropna()
            )
            ohlc.columns = ["open", "high", "low", "close"]
            ohlc = ohlc.reset_index()

            # Use string for better spacing and control
            ohlc["time_str"] = ohlc["time"].dt.strftime("%H:%M:%S")

            base = alt.Chart(ohlc).encode(
                x=alt.X("time_str:O", title="Time")
            )

            rule = base.mark_rule(color='black').encode(
                y="low:Q",
                y2="high:Q"
            )

            bar = base.mark_bar(size=8).encode(
                y="open:Q",
                y2="close:Q",
                color=alt.condition("datum.open <= datum.close",
                    alt.value("green"), alt.value("red"))
            )

            candlestick = (rule + bar).properties(width=800, height=400).configure_view(
                fill='white'
            ).configure_axis(
                grid=True,
                labelColor='black',
                titleColor='black'
            )

            st.altair_chart(candlestick, use_container_width=True)



    time.sleep(0.0001)
