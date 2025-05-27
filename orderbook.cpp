#include <iostream>
#include <set>
#include <vector>
#include<stdio.h>
#include<string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "KafkaProducer.h"
#include "orderbook.h"
using json = nlohmann::json;


long long getCurrentTimestampMillis() {
    using namespace std::chrono;
    milliseconds ms = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    );
    return ms.count();
}



bool OrderBook::addBuyOrder(std::string userId, int orderId, double price, int quantity, long long time) {
    Order order(userId, orderId, price, quantity, time); // creates an order
    auto result = buyOrders.insert(order);
    long long currentTimestamp = getCurrentTimestampMillis();
    
    RdKafka::Producer* producer = createKafkaProducer("localhost:9092");
    if (!producer) {
        std::cerr << "!!! Failed to create Kafka producer !!!" << std::endl;
        return false;
    }

     if (result.second) {
        // Insert succeeded (not a duplicate)
        buyOrderLookup[orderId] = result.first;


        // Kafka 
        json resultJson = toJson(_currentTradedPrice, buyOrders, sellOrders, currentTimestamp); // or lastTradePrice
        ProduceKafka(producer, "stock-details", resultJson.dump());
        producer->flush(5000);
        closeKafkaProducer(producer);

        return true;
     }
     else{
        std::cout<<"orderId"<<orderId <<" Exists already"<<std::endl;
        return false;
     }

}

bool OrderBook::addSellOrder(std::string userId, int orderId, double price, int quantity, long long time) {
    Order order(userId,orderId, price, quantity, time);
    long long currentTimestamp = getCurrentTimestampMillis();

    auto result = sellOrders.insert(order);
    RdKafka::Producer* producer = createKafkaProducer("localhost:9092");
    if (!producer) {
        std::cerr << "!!! Failed to create Kafka producer !!!" << std::endl;
        return false;
    }

    if (result.second) {
        // not a duplicate entry , a genuine entry.
        sellOrderLookup[orderId] = result.first;


         // Kafka 
        json resultJson = toJson(_currentTradedPrice, buyOrders, sellOrders, currentTimestamp); // or lastTradePrice
        ProduceKafka(producer, "stock-details", resultJson.dump());
        producer->flush(5000);
        closeKafkaProducer(producer);

        return true;
    } else {
        std::cout << "orderId " << orderId << " already exists\n";
        return false;
    }
}

bool OrderBook::cancelOrder(int OrderId, const OrderType &side){
long long currentTimestamp = getCurrentTimestampMillis();
RdKafka::Producer* producer = createKafkaProducer("localhost:9092");
if (!producer) {
        std::cerr << "!!! Failed to create Kafka producer !!!" << std::endl;
        return false;
    }
    if (side == OrderType::BUY)
{
    auto it = buyOrderLookup.find(OrderId);
    if (it != buyOrderLookup.end()) {
        Order cancelledOrder = *(it->second);

        // Remove from data structures
        buyOrders.erase(it->second);
        buyOrderLookup.erase(it);

        std::cout << "Buy order " << OrderId << " cancelled.\n";

        // Create and send cancellation notification
        json cancelNotification = createUserCancellationNotification(
            cancelledOrder.userId,    // Assuming your Order struct has userId
            cancelledOrder.orderId,
            "User-Side Cancellation",
            currentTimestamp
        );

        ProduceKafka(producer, "user.notifications", cancelNotification.dump(), cancelledOrder.userId);
            // Flush and close Kafka producer ONCE after all messages sent
            producer->flush(5000);
            closeKafkaProducer(producer);
        return true;
    }
    return false;
}


    else if (side == OrderType::SELL)
{
    auto it = sellOrderLookup.find(OrderId);
    if (it != sellOrderLookup.end())
    {
        Order cancelledOrder = *(it->second);

        // Erase from data structures
        sellOrders.erase(it->second);
        sellOrderLookup.erase(it);

        std::cout << "Sell order " << OrderId << " cancelled.\n";

        // Create and send cancellation notification
        json cancelNotification = createUserCancellationNotification(
            cancelledOrder.userId,     // Requires userId in Order struct
            cancelledOrder.orderId,
            " User - Side Cancellation",
            currentTimestamp
        );

        ProduceKafka(producer, "user.notifications", cancelNotification.dump(), cancelledOrder.userId);
            // Flush and close Kafka producer ONCE after all messages sent
    producer->flush(5000);
    closeKafkaProducer(producer);
        return true;
    }
        // Flush and close Kafka producer ONCE after all messages sent
    producer->flush(5000);
    closeKafkaProducer(producer);
    return false;
}

    return false;
}

void OrderBook::printOrders() {
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

bool OrderBook::eatSellOrder(std::string buyerUserId, int buyOrderId, double buyPrice, int buyQuantity, long long time)
{
    int remainingQuantity = buyQuantity;

    // Create Kafka producer ONCE outside the loop
    RdKafka::Producer* producer = createKafkaProducer("localhost:9092");
    if (!producer) {
        std::cerr << "!!! Failed to create Kafka producer !!!" << std::endl;
        return false;
    }

    for (auto it = sellOrders.begin(); it != sellOrders.end() && remainingQuantity > 0;)
    {
        const Order& sellSingleOrder = *it;

        if (sellSingleOrder.price <= buyPrice)
        {
            int tradeQuantity = std::min(remainingQuantity, sellSingleOrder.quantity);
            remainingQuantity -= tradeQuantity;

            long long currentTimestamp = getCurrentTimestampMillis();

            // Send notifications to SELLER and BUYER for this tradeQuantity

            // Seller notification
            json sellNotification = createUserTradeNotification(
                sellSingleOrder.userId,            // seller's userId
                sellSingleOrder.orderId,
                tradeQuantity,
                sellSingleOrder.price,
                currentTimestamp
            );
            ProduceKafka(producer, "trade-history", sellNotification.dump(), sellSingleOrder.userId);

            // Buyer notification
            json buyNotification = createUserTradeNotification(
                buyerUserId,                       // buyer's userId
                buyOrderId,
                tradeQuantity,
                sellSingleOrder.price,
                currentTimestamp
            );
            ProduceKafka(producer, "trade-history", buyNotification.dump(), buyerUserId);
            _currentTradedPrice = sellSingleOrder.price;

            std::cout<<" the last traded price is"<<_currentTradedPrice<<std::endl;

            

            if (tradeQuantity == sellSingleOrder.quantity)
            {
                // Fully eaten sell order — remove it
                sellOrderLookup.erase(sellSingleOrder.orderId);
                it = sellOrders.erase(it); // erase returns next iterator
            }
            else
            {
                // Partially eaten sell order — update quantity
                Order updatedOrder = sellSingleOrder;
                updatedOrder.quantity -= tradeQuantity;

                sellOrderLookup.erase(sellSingleOrder.orderId);
                it = sellOrders.erase(it);

                auto inserted = sellOrders.insert(updatedOrder);
                sellOrderLookup[updatedOrder.orderId] = inserted.first;
            }


            json resultJson = toJson(_currentTradedPrice, buyOrders, sellOrders, currentTimestamp); // or lastTradePrice
            ProduceKafka(producer, "stock-details", resultJson.dump());
        }
        else
        {
            // Price too high — stop matching
            break;
        }
    }

    // Flush and close Kafka producer ONCE after all messages sent
    producer->flush(5000);
    closeKafkaProducer(producer);

    if (remainingQuantity > 0)
    {
        return addBuyOrder(buyerUserId,buyOrderId, buyPrice, remainingQuantity, time);
    }

    return true;
}

bool OrderBook::eatBuyOrder(std::string sellerUserId, int sellOrderId, double sellPrice, int sellQuantity, long long time) {
    int remainingQty = sellQuantity;

    // Create Kafka producer once
    RdKafka::Producer* producer = createKafkaProducer("localhost:9092");
    if (!producer) {
        std::cerr << "!!! Failed to create Kafka producer !!!" << std::endl;
        return false;
    }

    for (auto it = buyOrders.begin(); it != buyOrders.end() && remainingQty > 0;) {
        const Order& buyOrder = *it;

        if (buyOrder.price >= sellPrice) {
            int tradedQty = std::min(remainingQty, buyOrder.quantity);
            remainingQty -= tradedQty;

            long long currentTimestamp = getCurrentTimestampMillis();

            // Notify buyer (buyOrder.userId) about trade
            json buyNotification = createUserTradeNotification(
                buyOrder.userId,
                buyOrder.orderId,
                tradedQty,
                buyOrder.price,
                currentTimestamp
            );
            ProduceKafka(producer, "trade-history", buyNotification.dump(), buyOrder.userId);

            // Notify seller (sellerUserId) about trade
            json sellNotification = createUserTradeNotification(
                sellerUserId,
                sellOrderId,
                tradedQty,
                buyOrder.price,
                currentTimestamp
            );
            ProduceKafka(producer, "trade-history", sellNotification.dump(), sellerUserId);
            _currentTradedPrice = buyOrder.price;
            std::cout<<" the last traded price is"<<_currentTradedPrice<<std::endl;
            

            if (tradedQty == buyOrder.quantity) {
                // Fully eaten buy order — erase
                buyOrderLookup.erase(buyOrder.orderId);
                it = buyOrders.erase(it);
            } else {
                // Partial consume buy order — erase and reinsert updated
                Order updatedBuyOrder = buyOrder;
                updatedBuyOrder.quantity -= tradedQty;

                buyOrderLookup.erase(buyOrder.orderId);
                it = buyOrders.erase(it);

                auto inserted = buyOrders.insert(updatedBuyOrder);
                buyOrderLookup[updatedBuyOrder.orderId] = inserted.first;
            }
            

            // ultimately Address the current price
           
            json resultJson = toJson(_currentTradedPrice, buyOrders, sellOrders, currentTimestamp); // or lastTradePrice
            ProduceKafka(producer, "stock-details", resultJson.dump());
        } else {
            // Price too low, stop matching
            break;
        }
    }

    // Flush and close Kafka producer once
    producer->flush(5000);
    closeKafkaProducer(producer);

    if (remainingQty > 0) {
        return addSellOrder(sellerUserId,sellOrderId, sellPrice, remainingQty, time);
    }

    return true;
}


bool OrderBook::buyMarketOrder(int buyOrderId, int quantity, long long time) {
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


bool OrderBook::sellMarketOrder(int sellOrderId, int quantity, long long time) {
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
    
    
bool OrderBook::buyStock(std::string userId, OrderMethod type, double price, int quantity,int orderId, long long timestamp)
    {
       if (type == OrderMethod::MarketOrder)
            return buyMarketOrder(orderId,quantity,timestamp);
       else
           return  eatSellOrder( userId,orderId,  price,  quantity, timestamp);

    }
bool OrderBook::sellStock(std::string userId, OrderMethod type, double price, int quantity, int orderId,long long timestamp)
    { if(type==OrderMethod::LimitOrder)
            std::cout<<"hasta khelta me";
         if (type == OrderMethod::MarketOrder)
            return sellMarketOrder(orderId,quantity,timestamp);
       else
            return eatBuyOrder(userId, orderId,  price,  quantity, timestamp);
    }

bool OrderBook::cancelBuyOrder(int orderId){
        return cancelOrder(orderId,OrderType::BUY);
    }
bool OrderBook::cancelSellOrder(int orderId){
        return cancelOrder(orderId,OrderType::SELL);
    }

json OrderBook::toJson(double lastTradedPrice,
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


json OrderBook::createUserTradeNotification(const std::string& userId,
                                 int orderId,
                                 int quantity,
                                 double exchangePrice,
                                 long long timestamp)
{
    json j;
    j["TransactionType"] = "Trade";
    j["userId"] = userId;
    j["orderId"] = orderId;
    j["quantity"] = quantity;
    j["exchangePrice"] = exchangePrice;
    j["timestamp"] = timestamp;
    return j;
}
json OrderBook::createUserCancellationNotification(const std::string& userId,
                                        int orderId,
                                        const std::string& reason,
                                        long long timestamp) {
    json j;
    j["TransactionType"] = "OrderCancelled";
    j["userId"] = userId;
    j["orderId"] = orderId;
    j["reason"] = reason;  // e.g., "UserRequested", "Expired", "FullyFilled"
    j["timestamp"] = timestamp;
    return j;
}







// int main() {
//     OrderBook ob;

//     long long currentTime = 1000;

//     // Sample userIds for testing
//     std::string userA = "userA";
//     std::string userB = "userB";
//     std::string userC = "userC";
//     std::string userD = "userD";

//     // Add buy limit orders with userIds (userId first!)
//     std::cout << "Adding buy limit orders:\n";
//     ob.buyStock(userA, OrderMethod::LimitOrder, 100.0, 10, 1, currentTime++);
//     ob.buyStock(userB, OrderMethod::LimitOrder, 101.0, 5, 2, currentTime++);
//     ob.buyStock(userC, OrderMethod::LimitOrder, 99.5, 20, 3, currentTime++);

//     // Add sell limit orders with userIds (userId first!)
//     std::cout << "Adding sell limit orders:\n";
//     ob.sellStock(userD, OrderMethod::LimitOrder, 102.0, 15, 4, currentTime++);
//     ob.sellStock(userA, OrderMethod::LimitOrder, 101.5, 10, 5, currentTime++);
//     ob.sellStock(userB, OrderMethod::LimitOrder, 103.0, 20, 6, currentTime++);

//     // This should NOT match yet (buy 98 < sell 101.5)
//     std::cout << "\nLimit buy that won't match (price too low):\n";
//     ob.buyStock(userC, OrderMethod::LimitOrder, 98.0, 10, 7, currentTime++);

//     // Add sell limit order at 100.5, which will match buy order at 101
//     std::cout << "\nSell at 100.5 — should match with buy @101 (ID 2):\n";
//     ob.sellStock(userD, OrderMethod::LimitOrder, 100.5, 5, 8, currentTime++);

//     // Add buy limit order at 101.5, should match with existing sell @101.5 (ID 5)
//     std::cout << "\nBuy at 101.5 — should match sell @101.5 (ID 5):\n";
//     ob.buyStock(userA, OrderMethod::LimitOrder, 101.5, 10, 9, currentTime++);

//     // Market buy order — should consume available lowest sell (ID 4 at 102.0)
//     std::cout << "\nMarket buy order (qty 12):\n";
//     ob.buyStock(userB, OrderMethod::MarketOrder, 0.0, 12, 10, currentTime++);

//     // Market sell order — should consume highest buy
//     std::cout << "\nMarket sell order (qty 8):\n";
//     ob.sellStock(userC, OrderMethod::MarketOrder, 0.0, 8, 11, currentTime++);

//     // Cancel a remaining buy order (ID 1)
//     std::cout << "\nCancel buy order with ID 1:\n";
//     if (ob.cancelBuyOrder(1))
//         std::cout << "Buy order 1 cancelled successfully\n";
//     else
//         std::cout << "Failed to cancel buy order 1\n";

//     // Cancel a remaining sell order (ID 6)
//     std::cout << "\nCancel sell order with ID 6:\n";
//     if (ob.cancelSellOrder(6))
//         std::cout << "Sell order 6 cancelled successfully\n";
//     else
//         std::cout << "Failed to cancel sell order 6\n";

//     return 0;
// }
