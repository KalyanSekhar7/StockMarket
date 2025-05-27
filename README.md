# StockMarket
This is a stock market simulation like platform starting from OrderBooks -> kafka(streaming) -> UserTrades -> dashboard.

# 🏦 Realistic Stock Market Simulation

This repository is a realistic, high-performance **Stock Market Simulation** system, designed to emulate the workings of a real trading platform. The core is built with C++ for speed and performance, enhanced with Kafka for real-time event streaming, and visualized with a Streamlit-based dashboard.

---

## 📈 Real-Time Candlestick View



Watch the live action as trades update the price candles in real time:

![Candle View](https://raw.githubusercontent.com/KalyanSekhar7/StockMarket/main/Screen%20Recording%20candle.gif)

## 📊 OrderBook View

![Stock Price](https://raw.githubusercontent.com/KalyanSekhar7/StockMarket/main/Screen%20Recording%20stock_price.gif)

---

## 📈 Stock Price View
![OrderBook](https://raw.githubusercontent.com/KalyanSekhar7/StockMarket/main/Screen%20Recording%20orderbook.gif)




## 🔧 Core Components

### 1. ⚙️ High-Performance OrderBook (C++)
- Located in: , `orderbook.cpp`, `priorityQueue.cpp`, `common.c`
- Optimized using priority queues to simulate a real-time exchange.

### 2. 🔄 Kafka-Based Event Streaming
- Kafka setup with **3 brokers** (1 Controller/Broker + 2 Brokers).
- The kafka brokers are setup using the docker containers `docker-compose.yml`
- The kafka version is the latest with KRaft ( No zookeeper)
- Data is streamed using:
  - `KafkaProducer.cpp` (C++ Kafka producer)
  - `kafkaConsumer.py` and `sample_kafka_consumer.*` (Python consumers)

### 3. 🎯 Trade Simulation
- `sample_user.py` simulates a stream of trades.
- Useful for testing and stress-loading the OrderBook.

### 4. 📊 Real-Time User Dashboard
- Implemented using `streamlitSampleUser.py`.
- Features:
  - Price chart and candlestick visualizations
  - Live Buy/Sell OrderBook
  - Real-time trade updates

---

## Prerequisits ( for C++ and Python)

### 1. The Python requirement
- The python requirements are listed in `requirements.txt`
- You can simply do `pip install -r requirements.txt`

### 2. C++ requirements

To build and run this project, you'll need the following C++ libraries:

- [`Crow`](https://github.com/CrowCpp/crow) – for exposing HTTP endpoints
- [`librdkafka`](https://github.com/edenhill/librdkafka) – Kafka client for C++
- [`nlohmann/json`](https://github.com/nlohmann/json) – modern C++ JSON library

---

### 💻 macOS

Install dependencies using Homebrew:

```bash
# Install Crow
brew install crow

# Install Kafka C++ Client
brew install librdkafka

# Install nlohmann JSON library
brew install nlohmann-json

```


### Linux (Ubuntu/Debian)
```
# Install Kafka C++ Client
sudo apt update
sudo apt install -y librdkafka-dev

# Install nlohmann JSON library
sudo apt install -y nlohmann-json-dev

# Install Crow (manual method)
git clone https://github.com/CrowCpp/crow.git
cd crow
sudo cp -r include/crow /usr/local/include/
```


- Make sure these are available after you install
```
#include <crow.h>                     // Crow HTTP framework
#include <librdkafka/rdkafkacpp.h>    // Kafka C++ client
#include <nlohmann/json.hpp>          // JSON for Modern C++
```


# Compile and run 
## 1. OderBook API

The core of this project is a high-performance C++ OrderBook engine, compiled into a single binary (`orderbook_api`) that handles trading logic and integrates with Kafka.

Use the appropriate compilation command based on your operating system.

---

###  macOS (Apple Silicon or Intel with Homebrew)

Make sure you've installed the dependencies via Homebrew as described earlier.

Then compile using:

```bash
clang++ -std=c++17 -g orderbook.cpp KafkaProducer.cpp api_endpoints.cpp \
  -o orderbook_api \
  -I/opt/homebrew/include \
  -L/opt/homebrew/lib \
  -lrdkafka++ -lrdkafka -lpthread
```


### Linux

compile using 

```bash
g++ -std=c++17 -g orderbook.cpp KafkaProducer.cpp api_endpoints.cpp \
  -o orderbook_api \
  -I/usr/include \
  -L/usr/lib \
  -lrdkafka++ -lrdkafka -lpthread

```

# - finally run your orderbook api 
  ```
./orderbook_api
```


## 2. KAFKA brokers
Starting Kafka Server with Docker Compose

- This project includes a pre-configured **Kafka setup with 3 brokers** using Docker Compose. To spin up the Kafka cluster locally:

```
docker-compose up -d
```

## 3. Streamlit for Dashboard

To run the streamlit applicaiton , you could simply

```streamlit run streamlitSampleUser.py```

- It would be running mostly on http://localhost:8501/ 
- Note: It would be running in some terminal / parallely ( since we have to run the simulation too)

## 4. Finally The simulation

To run the simulation

```python sample_user.py```

- check your streamlit appilication , for the stock price, orderBook , and the graphs.


# Finally 
## In short, three components are running simultaneously.
- OrderBook APi  ``` ./orderbook_api```
- streamlit dashboard ```streamlit run streamlitSampleUser.py```
- simulation ```python sample_user.py```







