#include <librdkafka/rdkafkacpp.h>
#include <iostream>
#include <csignal>

bool run = true;

void sigint_handler(int sig) {
    run = false;
}

class ExampleEventCb : public RdKafka::EventCb {
public:
    void event_cb(RdKafka::Event &event) override {
        std::cerr << "Kafka event: " << event.str() << std::endl;
    }
};

int main() {
    std::string brokers = "localhost:9092";
    std::string topic_name = "stock-details";
    std::string group_id = "stock-group";

    RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    RdKafka::Conf *tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
    std::string errstr;

    conf->set("bootstrap.servers", brokers, errstr);
    conf->set("group.id", group_id, errstr);

    ExampleEventCb ex_event_cb;
    conf->set("event_cb", &ex_event_cb, errstr);

    RdKafka::KafkaConsumer *consumer = RdKafka::KafkaConsumer::create(conf, errstr);
    if (!consumer) {
        std::cerr << "Failed to create consumer: " << errstr << std::endl;
        return 1;
    }

    consumer->subscribe({topic_name});

    signal(SIGINT, sigint_handler);
    std::cout << "Consuming messages from topic: " << topic_name << "\n";

    while (run) {
        RdKafka::Message *msg = consumer->consume(1000);
        switch (msg->err()) {
            case RdKafka::ERR_NO_ERROR:
                std::cout << "Received: " << static_cast<const char *>(msg->payload()) << std::endl;
                break;
            case RdKafka::ERR__TIMED_OUT:
                break;
            default:
                std::cerr << "Error: " << msg->errstr() << std::endl;
                run = false;
                break;
        }
        delete msg;
    }

    consumer->close();
    delete consumer;
    delete conf;
    delete tconf;

    return 0;
}
