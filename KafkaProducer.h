#pragma once

#include <librdkafka/rdkafkacpp.h>
#include <string>

// Initializes Kafka producer and returns a pointer
RdKafka::Producer* createKafkaProducer(const std::string& brokers);

// Sends JSON data to Kafka topic
void ProduceKafka(RdKafka::Producer* producer, const std::string& topic_name, const std::string& json_data,const std::string &key);
void ProduceKafka(RdKafka::Producer* producer, const std::string& topic_name, const std::string& json_data);

// Cleans up and flushes the Kafka producer
void closeKafkaProducer(RdKafka::Producer* producer);
