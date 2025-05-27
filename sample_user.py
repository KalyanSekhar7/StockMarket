import requests
import json
import time
import random

BASE_URL = "http://localhost:18080"
userId = "simUser"

HEADERS = {
    "Content-Type": "application/json"
}

def post_order(endpoint, payload):
    response = requests.post(f"{BASE_URL}/{endpoint}", data=json.dumps(payload), headers=HEADERS)
    print(f"[{endpoint.upper()}] Status: {response.status_code} | LimitOrder @ {payload['price']} Qty {payload['quantity']} | Resp: {response.text}")

def simulate_limit_trades(num_trades=50000, start_price=100.0):
    order_id = int(time.time() * 1000)
    current_price = start_price

    for i in range(num_trades):
        timestamp = int(time.time() * 1000)
        quantity = random.randint(10, 100)

        # Heavier fluctuations with bigger steps and wider range
        move = random.choices(
            population=["up", "down", "steady"],
            weights=[0.5, 0.3, 0.2],  # 50% up, 30% down, 20% steady
            k=1
        )[0]

        if move == "up":
            # Bigger upward jump up to +20 (can adjust)
            current_price += random.uniform(1.0, 20.0)
        elif move == "down":
            # Bigger downward jump up to -30 (can adjust)
            current_price -= random.uniform(1.0, 30.0)
            if current_price < 40.0:  # floor at 40
                current_price = 40.0

        # Cap upper bound at 200
        if current_price > 200.0:
            current_price = 200.0

        current_price = round(current_price, 2)

        # Sell price just above current price
        best_sell_price = current_price + random.uniform(0.1, 1.0)
        best_sell_price = round(best_sell_price, 2)

        # Buy price just below current price, no lower than 40
        best_buy_price = current_price - random.uniform(0.1, 1.0)
        if best_buy_price < 40.0:
            best_buy_price = 40.0
        best_buy_price = round(best_buy_price, 2)

        # SELL Limit order
        sell_payload = {
            "userId": f"{userId}_seller",
            "type": "LimitOrder",
            "price": best_sell_price,
            "quantity": quantity,
            "orderId": order_id,
            "timestamp": timestamp
        }
        post_order("sell", sell_payload)
        order_id += 1
        time.sleep(0.05)

        # BUY Limit order
        buy_payload = {
            "userId": f"{userId}_buyer",
            "type": "LimitOrder",
            "price": best_buy_price,
            "quantity": quantity,
            "orderId": order_id,
            "timestamp": int(time.time() * 1000)
        }
        post_order("buy", buy_payload)
        order_id += 1
        time.sleep(0.1)


simulate_limit_trades()
