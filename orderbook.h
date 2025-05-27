#pragma once

#include <iostream>
#include <set>
#include <unordered_map>
#include <string>
#include <nlohmann/json.hpp>
#include "KafkaProducer.h"

enum class OrderMethod { MarketOrder, LimitOrder };
enum class OrderType { SELL, BUY };

using json = nlohmann::json;

    struct Order {
        std::string userId;
        int orderId;
        double price;
        int quantity;
        long long time;

        Order(std::string uId, int id, double p, int q, long long t)
            : userId(uId), orderId(id), price(p), quantity(q), time(t) {}
    };

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



class OrderBook {
public:
    // Main trading APIs
    bool buyStock(std::string userId, OrderMethod type, double price, int quantity, int orderId, long long timestamp);
    bool sellStock(std::string userId, OrderMethod type, double price, int quantity, int orderId, long long timestamp);

    // Cancel APIs
    bool cancelBuyOrder(int orderId);
    bool cancelSellOrder(int orderId);

    // Print method (for debugging)
    void printOrders();

    // Snapshot of order book
    json toJson(double lastTradedPrice,
                const std::set<struct Order, struct BuyOrderComparator>& buyOrders,
                const std::set<struct Order, struct SellOrderComparator>& sellOrders,
                long long timestamp);
    // Kafka notifications
    json createUserTradeNotification(const std::string& userId,
                                     int orderId,
                                     int quantity,
                                     double exchangePrice,
                                     long long timestamp);

    json createUserCancellationNotification(const std::string& userId,
                                            int orderId,
                                            const std::string& reason,
                                            long long timestamp);

private:

    // State
    double _currentTradedPrice = 100;
    std::set<Order, BuyOrderComparator> buyOrders;
    std::set<Order, SellOrderComparator> sellOrders;
    std::unordered_map<int, std::set<Order, BuyOrderComparator>::iterator> buyOrderLookup;
    std::unordered_map<int, std::set<Order, SellOrderComparator>::iterator> sellOrderLookup;

    // Internal helpers
    bool addBuyOrder(std::string userId, int orderId, double price, int quantity, long long time);
    bool addSellOrder(std::string userId, int orderId, double price, int quantity, long long time);
    bool cancelOrder(int orderId, const OrderType& side);
    bool eatSellOrder(std::string buyerUserId, int buyOrderId, double buyPrice, int buyQuantity, long long time);
    bool eatBuyOrder(std::string sellerUserId, int sellOrderId, double sellPrice, int sellQuantity, long long time);
    bool buyMarketOrder(int buyOrderId, int quantity, long long time);
    bool sellMarketOrder(int sellOrderId, int quantity, long long time);

    
};

// Utility
long long getCurrentTimestampMillis();
