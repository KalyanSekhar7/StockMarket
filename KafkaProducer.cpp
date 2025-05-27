#include "KafkaProducer.h"
#include <iostream>
#include <csignal>

class ExampleDeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
    void dr_cb(RdKafka::Message &message) override {
        if (message.err())
            std::cerr << "❌ Delivery failed: " << message.errstr() << std::endl;
        else
            std::cout << "✅ Delivered to " << message.topic_name()
                      << " [" << message.partition() << "] at offset "
                      << message.offset() << std::endl;
    }
};

RdKafka::Producer* createKafkaProducer(const std::string& brokers) {
    std::string errstr;
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

    if (conf->set("bootstrap.servers", brokers, errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "❌ Config error: " << errstr << std::endl;
        return nullptr;
    }

    if (conf->set("compression.codec", "gzip", errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "❌ Compression config error: " << errstr << std::endl;
        return nullptr;
    }

    static ExampleDeliveryReportCb dr_cb;
    if (conf->set("dr_cb", &dr_cb, errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "❌ Callback config error: " << errstr << std::endl;
        return nullptr;
    }

    RdKafka::Producer* producer = RdKafka::Producer::create(conf, errstr);
    delete conf;

    if (!producer)
        std::cerr << "❌ Failed to create producer: " << errstr << std::endl;

    return producer;
}

void ProduceKafka(RdKafka::Producer* producer, const std::string& topic_name, const std::string& json_data,const std::string &key) {
    if (!producer) return;

    
    RdKafka::ErrorCode err = producer->produce(
        topic_name,
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char*>(json_data.c_str()),
        json_data.size(),
        key.data(),
        key.size(),
        0,       // timestamp, 0 means use broker timestamp
        nullptr  // msg_opaque
    );

    if (err != RdKafka::ERR_NO_ERROR) {
        std::cerr << "❌ Kafka error: " << RdKafka::err2str(err) << std::endl;
    } else {
        std::cout << "📤 Produced message to topic: " << topic_name << std::endl;
    }

    producer->poll(0);
}


void ProduceKafka(RdKafka::Producer* producer, const std::string& topic_name, const std::string& json_data) {
    if (!producer) return;

    
    RdKafka::ErrorCode err = producer->produce(
        topic_name,
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char*>(json_data.c_str()),
        json_data.size(),
        nullptr,
        0,0,
        nullptr
    );



    if (err != RdKafka::ERR_NO_ERROR) {
        std::cerr << "❌ Kafka error: " << RdKafka::err2str(err) << std::endl;
    } else {
        std::cout << "📤 Produced message to topic: " << topic_name << std::endl;
    }

    producer->poll(0);
}

void closeKafkaProducer(RdKafka::Producer* producer) {
    if (!producer) return;
    producer->flush(5000);
    delete producer;
}
