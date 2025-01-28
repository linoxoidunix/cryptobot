#pragma once
#include <unordered_set>
#include <string_view>
#include <toml++/toml.hpp>

#include "aot/Logger.h" 
#include "aot/common/types.h"

using namespace std::string_view_literals;
namespace config {
class IPathToPythonLib {
  public:
    /**
     * @brief bool - status, if true that is ok
     * std::string_view corresponded path
     */
    using Answer                     = std::pair<bool, std::string_view>;
    virtual Answer PathToPythonLib() = 0;
    virtual ~IPathToPythonLib()      = default;
};

class IPathToPythonModule {
  public:
    /**
     * @brief bool - status, if true that is ok
     * std::string_view corresponded path
     */
    using Answer                        = std::pair<bool, std::string_view>;
    virtual Answer PathToPythonModule() = 0;
    virtual ~IPathToPythonModule()      = default;
};

class IPathToHistoryData {
  public:
    /**
     * @brief bool - status, if true that is ok
     * std::string_view corresponded path
     */
    using Answer                       = std::pair<bool, std::string_view>;
    virtual Answer PathToHistoryData() = 0;
    virtual ~IPathToHistoryData()      = default;
};

class IApiKey{
  public:
    using Answer            = std::pair<bool, std::string_view>;
    virtual Answer ApiKey() = 0;
    virtual ~IApiKey()      = default;
};

class ISecretKey{
  public:
    using Answer            = std::pair<bool, std::string_view>;
    virtual Answer SecretKey() = 0;
    virtual ~ISecretKey()      = default;
};

class BackTesting : public IPathToPythonLib,
                    public IPathToPythonModule,
                    public IPathToHistoryData {
    static constexpr std::string_view kGeneralField    = "default";
    static constexpr std::string_view kPathToPythonLib = "path_to_python_lib";
    static constexpr std::string_view kPathToPythonModule =
        "path_to_python_module";
    static constexpr std::string_view kPathToHistoricalData =
        "path_to_history_data";

    toml::table config;

  public:
    explicit BackTesting(std::string_view path_to_toml);
    IPathToPythonLib::Answer PathToPythonLib() override;
    IPathToPythonModule::Answer PathToPythonModule() override;
    IPathToHistoryData::Answer PathToHistoryData() override;
    ~BackTesting() override = default;
  private:
    BackTesting()                                   = delete;
    BackTesting(const BackTesting& other)           = delete;
    BackTesting operator=(const BackTesting& other) = delete;
};

class ApiSecretKey : public IApiKey,
                    public ISecretKey{
    static constexpr std::string_view kExchangeField    = "exchange";
    static constexpr std::string_view kApiKey = "api_key";
    static constexpr std::string_view kSecretKey =  "secret_key";

    toml::table config;

  public:
    explicit ApiSecretKey(std::string_view path_to_toml);
    IApiKey::Answer ApiKey() override;
    ISecretKey::Answer SecretKey() override;
    ~ApiSecretKey() override = default;
  private:
    ApiSecretKey()                                   = delete;
    ApiSecretKey(const BackTesting& other)           = delete;
    ApiSecretKey operator=(const BackTesting& other) = delete;
};

/**
 * @class TickerManager
 * @brief A class responsible for managing tickers and their mappings.
 */
class TickerManager {
public:
    /**
     * @brief Adds a ticker to the ticker maps if it is not already present.
     * 
     * @param ticker The ticker symbol (e.g., "BTC", "USDT").
     * @param ticker_counter The counter to assign a unique integer index to the ticker.
     */
    void AddTicker(std::string_view ticker_sv, unsigned int& ticker_counter) {
        std::string ticker = ticker_sv.data();
        if (ticker_to_int.find(ticker) == ticker_to_int.end()) {
            ticker_to_int[ticker] = ticker_counter;
            int_to_ticker[ticker_counter] = ticker;
            ticker_counter++;
        }
    }
    std::pair<bool, unsigned int> TickerToInt(std::string_view ticker_sv){
       auto it = ticker_to_int.find(ticker_sv.data());
       if (it == ticker_to_int.end()) {
           return {false, 0};
       }
       return {true, it->second};
    }
    /**
     * @brief Prints the mapping from tickers to their integer indices.
     */
    void PrintTickerToInt() const {
        std::cout << "Ticker to int mapping:\n";
        for (const auto& entry : ticker_to_int) {
            std::cout << entry.first << " -> " << entry.second << "\n";
        }
    }

    /**
     * @brief Prints the mapping from integer indices to tickers.
     */
    void PrintIntToTicker() const {
        std::cout << "\nInt to ticker mapping:\n";
        for (const auto& entry : int_to_ticker) {
            std::cout << entry.first << " -> " << entry.second << "\n";
        }
    }

private:
    std::unordered_map<std::string, unsigned int> ticker_to_int; ///< Map from ticker symbol to unique integer index.
    std::unordered_map<unsigned int, std::string> int_to_ticker; ///< Map from integer index to ticker symbol.
};

/**
 * @class ITomlParser
 * @brief An interface for TOML file parsing.
 */
class ITomlParser {
public:
    virtual toml::table Parse(std::string_view filename) const = 0;
};

/**
 * @class TomlFileParser
 * @brief Concrete implementation of ITomlParser that uses toml++ for parsing.
 */
class TomlFileParser : public ITomlParser {
public:
    toml::table Parse(std::string_view filename) const override {
        toml::table config;
        try {
            config = toml::parse_file(filename);
        } catch (const std::exception& e) {
            throw std::runtime_error("Error parsing TOML file: " + std::string(e.what()));
        }
        return config;
    }
};

class TickerParser {
  class ExchangeConverter {
 public:
  // Converts ExchangeId to a string representation
  static std::string ToString(common::ExchangeId exchange_id) {
    switch (exchange_id) {
      case common::ExchangeId::kBinance:
        return "binance";
      case common::ExchangeId::kBybit:
        return "bybit";
      case common::ExchangeId::kMexc:
        return "mexc";
      default:
        return "invalid";
    }
  };
  };
 public:
  static constexpr const char* kExchangePrefix = "exchange";
  static constexpr const char* kTradingPairsPrefix = "trading_pairs";

  /**
   * @brief Constructor that initializes the dependencies for the parser.
   * 
   * @param file_parser The parser to read TOML files.
   * @param ticker_manager The manager responsible for handling tickers.
   * @param logger The logger to log messages.
   */
  TickerParser(std::shared_ptr<ITomlParser> file_parser,
               std::shared_ptr<TickerManager> ticker_manager)
      : file_parser_(std::move(file_parser)),
        ticker_manager_(std::move(ticker_manager)) {}

  /**
   * @brief Parses the provided TOML file to extract tickers and create mappings.
   * 
   * @param path_to_toml The path to the TOML file to be parsed.
   * @return true if parsing was successful, false if there was an error.
   */
  bool Parse(std::string_view path_to_toml) {
    try {
      auto config = LoadConfig(path_to_toml);
      auto list_exchanges = ParseExchanges(config);
      auto list_tickers = ParseTickers(config, list_exchanges);
      InitTickerManager(list_tickers);
      InitExchange(config, common::ExchangeId::kBinance);
      InitExchange(config, common::ExchangeId::kBybit);

      return true;
    } catch (const std::exception& e) {
      logi("Error during parsing: {}", std::string(e.what()));
    }
    return false;
  }
  common::TradingPairHashMap& PairsBinance(){
    return pairs_binance_;
  }
  common::TradingPairHashMap& PairsBybit(){
    return pairs_bybit_;
  }
 private:
  std::shared_ptr<ITomlParser> file_parser_;  // Dependency injection of TOML parser.
  std::shared_ptr<TickerManager> ticker_manager_;  // Dependency injection of ticker manager.
  common::TradingPairHashMap pairs_bybit_;
  common::TradingPairHashMap pairs_binance_;

  /**
   * @brief Loads the TOML configuration file.
   * 
   * @param path_to_toml The path to the TOML file to be parsed.
   * @return A parsed TOML configuration table.
   */
  toml::table LoadConfig(std::string_view path_to_toml) {
    return file_parser_->Parse(path_to_toml);
  }

  /**
   * @brief Parses exchanges from the TOML configuration and processes trading pairs.
   * 
   * @param config The parsed TOML configuration.
   * @param ticker_counter The counter used to assign unique indices to tickers.
   */
  std::unordered_set<std::string_view> ParseExchanges(const toml::table& config) {
      std::unordered_set<std::string_view> exchanges;
      if (auto* exchange_table = config["exchange"].as_table())
      {
          for (const auto& [key, _] : *exchange_table)
          {
              exchanges.insert(key);
          }
      }
      return exchanges;
  }
  std::unordered_set<std::string_view> ParseTickers(const toml::table& config, std::unordered_set<std::string_view> exchanges) {
      std::unordered_set<std::string_view> tickers;
      for(auto exchange : exchanges)
        if (auto* exchange_table = config[kTradingPairsPrefix][exchange].as_table())
        {
            for (const auto& [key, _] : *exchange_table)
            {
                tickers.insert(key);
                if (auto* table = config[kTradingPairsPrefix][exchange][key].as_table()){
                  for (const auto& [key_quote, _] : *table){
                      tickers.insert(key_quote);
                  }
                }
            }
        }
      return tickers;
  }
  void InitTickerManager(std::unordered_set<std::string_view> tickers) {
      if(!ticker_manager_)
        return;
      unsigned int counter = 0;
      for(auto ticker : tickers){
        ticker_manager_->AddTicker(ticker, counter);
      }
  }
  void InitExchange(const toml::table& config, common::ExchangeId type){
      auto exchange_as_string = ExchangeConverter::ToString(type);
      // Выбор нужной карты в зависимости от ExchangeId
      common::TradingPairHashMap* target_pairs = nullptr;
      switch (type) {
          case common::ExchangeId::kBybit:
              target_pairs = &pairs_bybit_;
              break;
          case common::ExchangeId::kBinance:
              target_pairs = &pairs_binance_;
              break;
          // Добавить другие биржи, если нужно
          default:
            break;
      }

      if (target_pairs == nullptr) {
          return;
      }
      auto& target_pairs_final = *target_pairs;
      if (auto* exchange_table = config[kTradingPairsPrefix][exchange_as_string].as_table()){
        for (const auto& [key, _] : *exchange_table){
            auto [status_base, base] = ticker_manager_->TickerToInt(key);
            if(!status_base)
              continue;
            if (auto* table = config[kTradingPairsPrefix][exchange_as_string][key].as_table()){
              for (const auto& [key_quote, value] : *table){
                auto [status_quote, quote] = ticker_manager_->TickerToInt(key_quote);
                if(!status_quote)
                  continue;
                  // Parse the additional fields
                int price_precision = 0;
                int qty_precision = 0;
                std::string_view https_json_request, https_query_request, ws_query_request, https_query_response;

                // Extract price_precision and qty_precision if they exist
                price_precision = (*table)[key_quote]["price_precision"].value_or(0);
                  
                qty_precision = (*table)[key_quote]["qty_precision"].value_or(0);

                // Extract the HTTP request values
                https_json_request = (*table)[key_quote]["https_json_request"].value_or(""sv);

                https_query_request = (*table)[key_quote]["https_query_request"].value_or(""sv);

                ws_query_request = (*table)[key_quote]["ws_query_request"].value_or(""sv);

                https_query_response = (*table)[key_quote]["https_query_response"].value_or(""sv);

                common::TradingPairInfo pair_info{
                  .price_precission     = static_cast<uint8_t>(price_precision),  // Price precision
                  .qty_precission       = static_cast<uint8_t>(qty_precision),  // Quantity precision
                  .https_json_request   = https_json_request.data(),  // HTTPS request for JSON responses
                  .https_query_request  = https_query_request.data(),  // HTTPS query request string
                  .ws_query_request     = ws_query_request.data(),  // WebSocket query request string
                  .https_query_response = https_query_response.data()   // HTTPS query response string
                };
                target_pairs_final[{base, quote}] = pair_info;
              }
            }
        }
      }

  }
};

};  // namespace config



