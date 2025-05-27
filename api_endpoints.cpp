#include <crow.h>
#include "orderbook.h"

int main() {
    crow::SimpleApp app;
    OrderBook orderBook;

    // Buy endpoint
    CROW_ROUTE(app, "/buy").methods(crow::HTTPMethod::POST)([&orderBook](const crow::request& req){
        auto body = json::parse(req.body);
        bool success = orderBook.buyStock(
            body["userId"],
            body["type"] == "LimitOrder" ? OrderMethod::LimitOrder : OrderMethod::MarketOrder,
            body["price"],
            body["quantity"],
            body["orderId"],
            body["timestamp"]
        );
        return crow::response(success ? 200 : 400, success ? "Order placed" : "Failed");
    });

    // Sell endpoint
    CROW_ROUTE(app, "/sell").methods(crow::HTTPMethod::POST)([&orderBook](const crow::request& req){
        auto body = json::parse(req.body);
        bool success = orderBook.sellStock(
            body["userId"],
            body["type"] == "LimitOrder" ? OrderMethod::LimitOrder : OrderMethod::MarketOrder,
            body["price"],
            body["quantity"],
            body["orderId"],
            body["timestamp"]
        );
        return crow::response(success ? 200 : 400, success ? "Order placed" : "Failed");
    });

    // Cancel endpoint
    CROW_ROUTE(app, "/cancel").methods(crow::HTTPMethod::POST)([&orderBook](const crow::request& req){
        auto body = json::parse(req.body);
        bool success = false;
        std::string type = body["side"];
        if (type == "BUY")
            success = orderBook.cancelBuyOrder(body["orderId"]);
        else if (type == "SELL")
            success = orderBook.cancelSellOrder(body["orderId"]);
        return crow::response(success ? 200 : 400, success ? "Order cancelled" : "Failed to cancel");
    });

    // View orderbook snapshot
    // CROW_ROUTE(app, "/orderbook").methods(crow::HTTPMethod::GET)([&orderBook]{
    //     auto now = getCurrentTimestampMillis();
    //     json snapshot = orderBook.toJson(/* lastTradedPrice */ 0.0, orderBook.buyOrders, orderBook.sellOrders, now);
    //     return crow::response(snapshot.dump());
    // });

    app.port(18080).multithreaded().run();
}
