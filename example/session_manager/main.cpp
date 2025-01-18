#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <functional>

class SubscriptionManager {
public:
    using CallbackType = std::function<void(const std::string&)>;

    void Subscribe(const std::string& subscription, const std::string& pair, CallbackType callback) {
        std::cout << "Subscribed to " << pair << " under " << subscription << std::endl;
        callback(pair);  // Пример вызова callback
    }

    bool HasSubscription(const std::string& subscription, const std::string& pair) {
        // Пример проверки, есть ли такая подписка
        return subscription == "sub1" && pair == "BTCUSDT";  // Пример логики
    }
};

class WebSession {
public:
    void SetSubscriptionManager(std::shared_ptr<SubscriptionManager> manager) {
        subscriptionManager = manager;
    }

    std::shared_ptr<SubscriptionManager> GetSubscriptionManager() {
        return subscriptionManager;
    }

private:
    std::shared_ptr<SubscriptionManager> subscriptionManager;
};

class SessionPool {
public:
    size_t Size() const { return sessions.size(); }
    std::shared_ptr<WebSession> GetSession(size_t index) {
        if (index < sessions.size()) {
            return sessions[index];
        }
        return nullptr;
    }

    void AddSession(std::shared_ptr<WebSession> session) {
        sessions.push_back(session);
    }

private:
    std::vector<std::shared_ptr<WebSession>> sessions;
};

class SubscriptionDistributor {
public:
    SubscriptionDistributor(SessionPool& pool) : pool_(pool) {}

    void SubscribeToSession(const std::string& subscription, const std::string& pair, 
                             SubscriptionManager::CallbackType callback) {
        if (pool_.Size() == 0) {
            std::cout << "No available sessions!" << std::endl;
            return;
        }

        // Атомарная проверка, была ли уже подписка
        std::pair<std::string, std::string> subscriptionKey = {subscription, pair};
        if (subscriptionSessionMap_.find(subscriptionKey) != subscriptionSessionMap_.end()) {
            std::cout << "Subscription already exists for " << pair << " under " << subscription << std::endl;
            return;  // Если подписка уже существует, прекращаем выполнение
        }

        // Используем хэш-функцию для распределения подписки
        size_t session_index = Hash(subscription, pair) % pool_.Size();
        auto session = pool_.GetSession(session_index);

        if (session) {
            std::shared_ptr<SubscriptionManager> manager = std::make_shared<SubscriptionManager>();
            session->SetSubscriptionManager(manager);
            manager->Subscribe(subscription, pair, callback);

            // Добавляем информацию о подписке в атомарную мапу
            subscriptionSessionMap_[subscriptionKey] = true;  // Устанавливаем флаг существования подписки
        }
    }

private:
    SessionPool& pool_;

    // Атомарная мапа для отслеживания подписок
    std::unordered_map<std::pair<std::string, std::string>, bool> subscriptionSessionMap_;  

    size_t Hash(const std::string& subscription, const std::string& pair) {
        return std::hash<std::string>{}(subscription + pair);
    }
};

int main() {
    // Создаем пул сессий
    SessionPool pool;
    auto session1 = std::make_shared<WebSession>(0);
    auto session2 = std::make_shared<WebSession>(1);
    auto session3 = std::make_shared<WebSession>(2);

    pool.AddSession(session1);
    pool.AddSession(session2);
    pool.AddSession(session3);

    SubscriptionDistributor distributor(pool);

    // Подписка на пару
    distributor.SubscribeToSession("DEPTH", "BTCUSDT", [](const std::string& response) {
        std::cout << "Callback DEPTH received: " << response << std::endl;
    });
    distributor.SubscribeToSession("DEPTH", "ETHUSDT", [](const std::string& response) {
        std::cout << "Callback DEPTH received: " << response << std::endl;
    });
    distributor.SubscribeToSession("SNAPSHOT", "BTCUSDT", [](const std::string& response) {
        std::cout << "Callback SNAPSHOT received: " << response << std::endl;
    });
    session1->CallCallbacks();
    session2->CallCallbacks();
    session3->CallCallbacks();

    return 0;
}

