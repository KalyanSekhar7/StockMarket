# kafka_consumer.py
from kafka import KafkaConsumer
import pandas as pd
import json
import time

consumer = KafkaConsumer(
    'stock-details',
    bootstrap_servers='localhost:9092',
    auto_offset_reset='latest',
    value_deserializer=lambda m: json.loads(m.decode('utf-8'))
)

def get_latest_data():
    for message in consumer:
        return message.value



