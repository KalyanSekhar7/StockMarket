#include <iostream>
#include <set>
#include <vector>
#include<stdio.h>
#include<string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "KafkaProducer.h"
#include "json.hpp" // If using nlohmann::json
using json = nlohmann::json;



// Somewhere in your logic:
RdKafka::Producer* producer = createKafkaProducer("localhost:9092");

// if (producer) {
//     json resultJson = toJson(lastTradedPrice, buyOrders, sellOrders, currentTimestamp);
//     ProduceKafka(producer, "stock-details", resultJson.dump());
//     closeKafkaProducer(producer);
// }

struct Order {
    int orderId;
    double price;
    int quantity;
    long long time;

    Order(int id, double p, int q, long long t)
        : orderId(id), price(p), quantity(q), time(t) {}
};

enum class OrderType{
    SELL,
    BUY
};
// Buy order comparator: higher price → higher quantity → earlier time
struct BuyOrderComparator {
    bool operator()(const Order& a, const Order& b) const {
        if (a.price != b.price)
            return a.price > b.price; // higher price first
        if (a.quantity != b.quantity)
            return a.quantity > b.quantity; // higher quantity first
        if (a.time != b.time)
            return a.time < b.time; // earlier time first
        return a.orderId < b.orderId; // ensure uniqueness in set
    }
};

// Sell order comparator: lower price → higher quantity → earlier time
struct SellOrderComparator {
    bool operator()(const Order& a, const Order& b) const {
        if (a.price != b.price)
            return a.price < b.price; // lower price first
        if (a.quantity != b.quantity)
            return a.quantity > b.quantity; // higher quantity first
        if (a.time != b.time)
            return a.time < b.time; // earlier time first
        return a.orderId < b.orderId; // ensure uniqueness in set
    }
};

std::set<Order, BuyOrderComparator> buyOrders;
// this is helpful for cancelling the order in the first place. 
std::unordered_map<int, std::set<Order, BuyOrderComparator>::iterator> buyOrderLookup;

std::set<Order, SellOrderComparator> sellOrders;
std::unordered_map<int, std::set<Order, SellOrderComparator>::iterator> sellOrderLookup;

json toJson(double lastTradedPrice,
            const std::set<Order, BuyOrderComparator>& buyOrders,
            const std::set<Order, SellOrderComparator>& sellOrders,
            long long timestamp);

bool addBuyOrder(int orderId, double price, int quantity, long long time) {
    //buyOrders.emplace(orderId, price, quantity, time);
    Order order(orderId, price, quantity, time); // creates an order
    auto result = buyOrders.insert(order);

     if (result.second) {
        // Insert succeeded (not a duplicate)
        buyOrderLookup[orderId] = result.first;
        return true;
     }
     else{
        std::cout<<"orderId"<<orderId <<" Exists already"<<std::endl;
        return false;
     }

}

bool addSellOrder(int orderId, double price, int quantity, long long time) {
    Order order(orderId, price, quantity, time);
    auto result = sellOrders.insert(order);
    if (result.second) {
        // not a duplicate entry , a genuine entry.
        sellOrderLookup[orderId] = result.first;
        return true;
    } else {
        std::cout << "orderId " << orderId << " already exists\n";
        return false;
    }
}

bool cancelOrder(int OrderId, const OrderType &side){

    if (side  == OrderType::BUY)
    {
        auto it = buyOrderLookup.find(OrderId);
        if (it != buyOrderLookup.end()) {
            buyOrders.erase(it->second);
            buyOrderLookup.erase(it);
            std::cout << "Buy order " << OrderId << " cancelled.\n";
            return true;
        }
        return false;
    }

    else if (side == OrderType::SELL)
    {
        auto it = sellOrderLookup.find(OrderId);
        if (it!= sellOrderLookup.end())
        {
            sellOrders.erase(it->second); // it second  = value basially , we take the iterator of the SellOrder and use that to delete faster
            sellOrderLookup.erase(it);
            std::cout << "Sell order " << OrderId << " cancelled.\n";
            return true;
        }
        return false;
    }
    return false;
}

void printOrders() {
    std::cout << "\n=== Buy Orders ===\n";
    for (const auto& order : buyOrders) {
        std::cout << "ID: " << order.orderId << " Price: " << order.price
                  << " Qty: " << order.quantity << " Time: " << order.time << '\n';
    }

    std::cout << "\n=== Sell Orders ===\n";
    for (const auto& order : sellOrders) {
        std::cout << "ID: " << order.orderId << " Price: " << order.price
                  << " Qty: " << order.quantity << " Time: " << order.time << '\n';
    }
}

bool eatSellOrder(std::string userId, int buyOrderId, double buyPrice, int buyQuantity, long long time)
{
    int remainingQuantity = buyQuantity;

    // Now we iterate over sell orders

    for (auto it = sellOrders.begin();it!= sellOrders.end() && remainingQuantity>0;it++)
    {
        const Order sellSingleOrder = *it;



        // kafka broker should be active , otherwise traders wont know wth happened to their stock
   

        if (sellSingleOrder.price<=buyPrice)
            {
                // Trade in progress
                int tradeQuantity = std::min(remainingQuantity,sellSingleOrder.quantity);

                remainingQuantity -=  tradeQuantity; 

                if (tradeQuantity == sellSingleOrder.quantity)
                {// delete that order
                    sellOrderLookup.erase(sellSingleOrder.orderId);
                    sellOrders.erase(it);

                }

                else{
                    //partially eaten the order
                    Order updateSellSingleOrder = sellSingleOrder;
                    updateSellSingleOrder.quantity -= tradeQuantity;

                    // we need to earse and then add this orderAgain

                    sellOrderLookup.erase(sellSingleOrder.orderId);
                    it = sellOrders.erase(it);  // once it erases, it points towars the next item do it otherwise , it will point to invalid pointer

                    // add back again
                    auto inserted = sellOrders.insert(updateSellSingleOrder);
                    sellOrderLookup[updateSellSingleOrder.orderId] = inserted.first;


                }
                RdKafka::Producer* producer = createKafkaProducer("localhost:9092");
                if (producer) {
                        json resultJson = toJson(buyPrice, buyOrders, sellOrders, 12345);
                        ProduceKafka(producer, "stock-details", resultJson.dump());
                        closeKafkaProducer(producer);
                        }
            }
        else
        {
            // buy was too low , now break and add it to the prderBook
            break;
        }
    }



    if (remainingQuantity>0)
    {
      
        return addBuyOrder(buyOrderId, buyPrice, remainingQuantity, time);
    
    }
    // it means , it was eaten by the SellOrderBook
    return true;
}


bool eatBuyOrder(std::string userId, int sellOrderId, double sellPrice, int sellQuantity, long long time) {
    int remainingQty = sellQuantity;

    // Iterate over buyOrders (highest price first)
    for (auto it = buyOrders.begin(); it != buyOrders.end() && remainingQty > 0;) {
        const Order& buyOrder = *it;

        // Only match if buy price >= sell price
        if (buyOrder.price >= sellPrice) {
            int tradedQty = std::min(remainingQty, buyOrder.quantity);
            std::cout << "Trade executed: SellID " << sellOrderId
                      << " sells " << tradedQty << " units @ " << buyOrder.price
                      << " to BuyID " << buyOrder.orderId << std::endl;

            remainingQty -= tradedQty;

            if (tradedQty == buyOrder.quantity) {
                // Fully consumed buy order, erase it
                buyOrderLookup.erase(buyOrder.orderId);
                it = buyOrders.erase(it);
            } else {
                // Partially consume buy order - erase and reinsert with updated quantity
                Order updatedBuyOrder = buyOrder;
                updatedBuyOrder.quantity -= tradedQty;

                buyOrderLookup.erase(buyOrder.orderId);
                it = buyOrders.erase(it);

                auto inserted = buyOrders.insert(updatedBuyOrder);
                buyOrderLookup[updatedBuyOrder.orderId] = inserted.first;
            }
            RdKafka::Producer* producer = createKafkaProducer("localhost:9092");
            if (producer) {
                        json resultJson = toJson(sellPrice, buyOrders, sellOrders, 12345);
                        ProduceKafka(producer, "stock-details", resultJson.dump());
                        closeKafkaProducer(producer);
                        }
        } else {
            // Buy price too low to match
            break;
        }
    }

    // If there is leftover sell quantity, add it to sellOrders
   if (remainingQty > 0) {
        return addSellOrder(sellOrderId, sellPrice, remainingQty, time);
    }

    // Fully matched, no leftover
    return true;
}


bool buyMarketOrder(int buyOrderId, int quantity, long long time) {
    int remainingQty = quantity;

    for (auto it = sellOrders.begin(); it != sellOrders.end() && remainingQty > 0;) {
        const Order& sellOrder = *it;
        int tradedQty = std::min(remainingQty, sellOrder.quantity);

        std::cout << "Market Buy: BuyID " << buyOrderId << " buys " << tradedQty
                  << " units @ " << sellOrder.price << " from SellID " << sellOrder.orderId << std::endl;

        remainingQty -= tradedQty;

        if (tradedQty == sellOrder.quantity) {
            sellOrderLookup.erase(sellOrder.orderId);
            it = sellOrders.erase(it);
        } else {
            Order updatedSellOrder = sellOrder;
            updatedSellOrder.quantity -= tradedQty;

            sellOrderLookup.erase(sellOrder.orderId);
            it = sellOrders.erase(it);

            auto inserted = sellOrders.insert(updatedSellOrder);
            sellOrderLookup[updatedSellOrder.orderId] = inserted.first;
        }
    }

    if (remainingQty > 0) {
        std::cout << "Market Buy Order " << buyOrderId << " partially filled, " << remainingQty << " units remain unfilled\n";
        // Usually market orders don't add leftovers to book, so just return false
        return false;
    }
    return true;
}


bool sellMarketOrder(int sellOrderId, int quantity, long long time) {
    int remainingQty = quantity;

    for (auto it = buyOrders.begin(); it != buyOrders.end() && remainingQty > 0;) {
        const Order& buyOrder = *it;
        int tradedQty = std::min(remainingQty, buyOrder.quantity);

        std::cout << "Market Sell: SellID " << sellOrderId << " sells " << tradedQty
                  << " units @ " << buyOrder.price << " to BuyID " << buyOrder.orderId << std::endl;

        remainingQty -= tradedQty;

        if (tradedQty == buyOrder.quantity) {
            buyOrderLookup.erase(buyOrder.orderId);
            it = buyOrders.erase(it);
        } else {
            Order updatedBuyOrder = buyOrder;
            updatedBuyOrder.quantity -= tradedQty;

            buyOrderLookup.erase(buyOrder.orderId);
            it = buyOrders.erase(it);

            auto inserted = buyOrders.insert(updatedBuyOrder);
            buyOrderLookup[updatedBuyOrder.orderId] = inserted.first;
        }
    }

    if (remainingQty > 0) {
        std::cout << "Market Sell Order " << sellOrderId << " partially filled, " << remainingQty << " units remain unfilled\n";
        return false;
    }
    return true;
}





json toJson(double lastTradedPrice,
            const std::set<Order, BuyOrderComparator>& buyOrders,
            const std::set<Order, SellOrderComparator>& sellOrders,
            long long timestamp)
{
    json j;

    j["lastTradedPrice"] = lastTradedPrice;
    j["timestamp"] = timestamp;

    // Convert buyOrders to JSON array
    json buyArray = json::array();
    for (const auto& order : buyOrders) {
        buyArray.push_back({
            {"orderId", order.orderId},
            {"price", order.price},
            {"quantity", order.quantity},
            {"time", order.time}
        });
    }
    j["buyTable"] = buyArray;

    // Convert sellOrders to JSON array
    json sellArray = json::array();
    for (const auto& order : sellOrders) {
        sellArray.push_back({
            {"orderId", order.orderId},
            {"price", order.price},
            {"quantity", order.quantity},
            {"time", order.time}
        });
    }
    j["sellTable"] = sellArray;

    return j;
}

int main() {
    // Simulated time counter
    long long currentTime = 1000;

    // Simulate incoming Buy Orders (try to match Sell side first)
    eatSellOrder(1, 100.0, 10, currentTime++);  // Likely added to book (no match yet)
    eatSellOrder(2, 101.0, 5, currentTime++);   // Likely added to book
    eatSellOrder(3, 101.0, 20, currentTime++);  // Higher quantity
    eatSellOrder(4, 100.0, 10, currentTime++);  // Same price/qty as ID 1, later time

    // Simulate incoming Sell Orders (try to match Buy side first)
    eatBuyOrder(5, 102.0, 15, currentTime++);   // Too expensive, added to book
    eatBuyOrder(6, 101.0, 10, currentTime++);   // May match
    eatBuyOrder(7, 101.0, 20, currentTime++);   // Higher quantity
    eatBuyOrder(8, 102.0, 15, currentTime++);   // Same price as ID 5, later

    std::cout << "\n--- Initial Order Book ---\n";
    printOrders();


double lastTradedPrice = 101.5;
long long currentTimestamp = 1234567890;

json resultJson = toJson(lastTradedPrice, buyOrders, sellOrders, currentTimestamp);
std::cout << resultJson.dump(4) << std::endl; // Pretty print with indent=4
    return 0;
}
