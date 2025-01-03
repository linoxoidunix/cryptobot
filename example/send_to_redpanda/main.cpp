// #include <iostream>
// #include <string>
// #include <nlohmann/json.hpp>
// #include <cppkafka/cppkafka.h>
// #include <thread>
// #include <vector>
// #include <cstdlib>
// #include <ctime>

// using json = nlohmann::json;

// // Wallet class to represent wallet data
// class Wallet {
// public:
//     Wallet(const std::string& exchange, const std::string& ticker, double balance)
//         : exchange(exchange), ticker(ticker), balance(balance) {}

//     // Serialize to JSON
//     json to_json() const {
//         return json{
//             {"wallet", {
//                 {"exchange", exchange},
//                 {"ticker", ticker},
//                 {"balance", balance}
//             }}
//         };
//     }

// private:
//     std::string exchange;
//     std::string ticker;
//     double balance;
// };

// // Pnl class to represent profit and loss data
// class Pnl {
// public:
//     Pnl(const std::string& exchange, const std::string& tradingPair, double realized, double unrealized)
//         : exchange(exchange), tradingPair(tradingPair), realized(realized), unrealized(unrealized) {}

//     // Serialize to JSON
//     json to_json() const {
//         return json{
//             {"pnl", {
//                 {"exchange", exchange},
//                 {"trading_pair", tradingPair},
//                 {"realized", realized},
//                 {"unrealized", unrealized}
//             }}
//         };
//     }

// private:
//     std::string exchange;
//     std::string tradingPair;
//     double realized;
//     double unrealized;
// };

// // OrderBook class to represent market order book data
// class OrderBook {
// public:
//     OrderBook(const std::string& exchange, const std::string& tradingPair, double bestBid, double bestOffer, double spread)
//         : exchange(exchange), tradingPair(tradingPair), bestBid(bestBid), bestOffer(bestOffer), spread(spread) {}

//     // Serialize to JSON
//     json to_json() const {
//         return json{
//             {"orderbook", {
//                 {"exchange", exchange},
//                 {"trading_pair", tradingPair},
//                 {"best_bid", bestBid},
//                 {"best_offer", bestOffer},
//                 {"spread", spread}
//             }}
//         };
//     }

// private:
//     std::string exchange;
//     std::string tradingPair;
//     double bestBid;
//     double bestOffer;
//     double spread;
// };

// class KafkaClient {
// public:
//     KafkaClient(const std::string& brokers) {
//         // Kafka configuration
//         cppkafka::Configuration config = {
//             { "metadata.broker.list", brokers }
//         };

//         // Create and store the producer as a member
//         producer_ = std::make_unique<cppkafka::Producer>(config);
//     }

//     cppkafka::Producer& get_producer() {
//         return *producer_;  // Return by reference
//     }

// private:
//     std::unique_ptr<cppkafka::Producer> producer_;  // Store producer as a member
// };

// // Function to produce messages to Kafka/Redpanda asynchronously
// template <typename T>
// void produce_message(cppkafka::Producer& producer, const std::string& topic, const T& object) {
//     // Serialize the object to JSON
//     json j = object.to_json();
//     std::string payload = j.dump();

//     // Create Kafka message
//     cppkafka::MessageBuilder message(topic);
//     message.payload(payload);

//     // Send message to the topic asynchronously
//     producer.produce(message);
//     std::cout << "Message sent to topic " << topic << ": " << payload << std::endl;
// }

// // Function to generate random orderbook data
// OrderBook generate_random_orderbook(const std::vector<std::string>& exchanges, const std::vector<std::string>& tickers) {
//     // Randomly select an exchange
//     std::string exchange = exchanges[rand() % exchanges.size()];

//     // Randomly select two different tickers to form a trading pair
//     std::string ticker1 = tickers[rand() % tickers.size()];
//     std::string ticker2;
//     do {
//         ticker2 = tickers[rand() % tickers.size()];
//     } while (ticker1 == ticker2);
//     std::string tradingPair = ticker1 + "/" + ticker2;

//     // Generate random best bid, best offer, and spread
//     double bestBid = 50000.0 + (rand() % 1000);
//     double bestOffer = bestBid + (rand() % 50);
//     double spread = bestOffer - bestBid;

//     return OrderBook(exchange, tradingPair, bestBid, bestOffer, spread);
// }

// // Function to generate random Pnl data
// Pnl generate_random_pnl(const std::vector<std::string>& exchanges, const std::vector<std::string>& tickers) {
//     // Randomly select an exchange
//     std::string exchange = exchanges[rand() % exchanges.size()];

//     // Randomly select two different tickers to form a trading pair
//     std::string ticker1 = tickers[rand() % tickers.size()];
//     std::string ticker2;
//     do {
//         ticker2 = tickers[rand() % tickers.size()];
//     } while (ticker1 == ticker2);
//     std::string tradingPair = ticker1 + "/" + ticker2;

//     // Generate random realized and unrealized PnL
//     double realizedPnL = (rand() % 2000) - 1000;  // Random values between -1000 and 1000
//     double unrealizedPnL = (rand() % 2000) - 1000;

//     return Pnl(exchange, tradingPair, realizedPnL, unrealizedPnL);
// }

// // Function to generate random Wallet data
// Wallet generate_random_wallet(const std::vector<std::string>& exchanges, const std::vector<std::string>& tickers) {
//     // Randomly select an exchange
//     std::string exchange = exchanges[rand() % exchanges.size()];

//     // Randomly select a ticker
//     std::string ticker = tickers[rand() % tickers.size()];

//     // Generate random balance
//     double balance = static_cast<double>(rand() % 10000) / 100.0;  // Random balance up to 10000.00

//     return Wallet(exchange, ticker, balance);
// }

// // Function to send multiple messages to orderbook, pnl, and wallet topics
// void send_multiple_messages(cppkafka::Producer& producer, const std::string& orderbookTopic, const std::string& pnlTopic, const std::string& walletTopic, int numMessages) {
//     std::vector<std::string> exchanges = {"Binance", "Coinbase", "Kraken", "Bitfinex", "Huobi"};
//     std::vector<std::string> tickers = {"BTC", "ETH", "XRP", "LTC", "ADA"};

//     for (int i = 0; i < numMessages; ++i) {
//         // Generate random OrderBook, Pnl, and Wallet objects
//         OrderBook orderBook = generate_random_orderbook(exchanges, tickers);
//         Pnl pnl = generate_random_pnl(exchanges, tickers);
//         Wallet wallet = generate_random_wallet(exchanges, tickers);

//         // Send messages to their respective topics
//         produce_message(producer, orderbookTopic, orderBook);
//         produce_message(producer, pnlTopic, pnl);
//         produce_message(producer, walletTopic, wallet);
//     }
// }

// int main() {
//     // Initialize random seed
//     srand(static_cast<unsigned>(time(0)));

//     // Kafka/Redpanda broker configuration
//     std::string brokers = "localhost:19092";  // Specify your Redpanda broker address here

//     // Create Kafka Producer
//     KafkaClient kafka_client(brokers);
//     cppkafka::Producer& producer = kafka_client.get_producer();

//     // Send multiple messages to orderbook, pnl, and wallet topics
//     send_multiple_messages(producer, "orderbook", "pnl", "wallet", 100);

//     // Handle events and wait for all messages to be sent
//     producer.flush(); // Wait until all messages are delivered

//     return 0;
// }

#include "aot/redpanda_client/redpanda_client.h"
#include "aot/proto/classes/proto_orderbook.h"
#include <string_view>

int main(){
    std::string brokers = "localhost:19092";  // Specify your Redpanda broker address here
    aot::KafkaClient client(brokers);
    
    std::string_view exchange = "binance";
    std::string_view trading_pair = "btc/usdt";
    aot::models::OrderBook ob(exchange, trading_pair, 450, 500, 50);
    
    client.SendMessage(ob);
    client.WaitUntilFinished();
    return 0;
}