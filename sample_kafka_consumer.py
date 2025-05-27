from confluent_kafka import Consumer, KafkaException, KafkaError

# Kafka configuration
conf = {
    'bootstrap.servers': 'localhost:9092',  # Matches PLAINTEXT_HOST
    'group.id': 'stock-consumer-group',
    'auto.offset.reset': 'earliest'
}

# Create Consumer instance
consumer = Consumer(conf)

# Subscribe to topic
consumer.subscribe(['stock-details'])

print("📥 Listening for messages on 'stock-details'...")

try:
    while True:
        msg = consumer.poll(timeout=1.0)
        if msg is None:
            continue  # No message yet
        if msg.error():
            if msg.error().code() != KafkaError._PARTITION_EOF:
                print("❌ Kafka error:", msg.error())
                raise KafkaException(msg.error())
            continue
        # Message received
        print(f"✅ Received message: {msg.value().decode('utf-8')}")

except KeyboardInterrupt:
    print("\n🛑 Stopped by user")

finally:
    consumer.close()
