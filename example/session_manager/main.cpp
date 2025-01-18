#include <string_view>
#include <vector>
#include <functional>
#include <string>
#include <iostream>
#include <memory>
#include <atomic>
#include <unordered_set>

class WebSession;

class SubscriptionManager {
public:
    using SubscriptionType = std::string;
    using PairType = std::string;
    using CallbackType = std::function<void(const std::string&)>;

private:
    std::unordered_map<SubscriptionType, std::unordered_set<PairType>> subscriptions;
    std::unordered_map<SubscriptionType, CallbackType> callbacks;

public:
    // Подписка
    void Subscribe(const SubscriptionType& subscription, const PairType& pair, CallbackType callback) {
        if (callbacks.count(subscription)) {
            subscriptions[subscription].emplace(pair);
            std::cout << "Already subscribed to " << pair << " under subscription: " << subscription << std::endl;
            return;
        }
        callbacks[subscription] = callback;
        subscriptions[subscription].emplace(pair);
    }

    // Отписка
    void Unsubscribe(const SubscriptionType& subscription, const PairType& pair) {
        auto it = subscriptions.find(subscription);
        if (it != subscriptions.end()) {
            auto& pairs = it->second;
            if (pairs.erase(pair) > 0) {
                std::cout << "Unsubscribed: " << subscription << " -> " << pair << std::endl;
                if (pairs.empty()) {
                    subscriptions.erase(subscription);
                    callbacks.erase(subscription);
                    std::cout << "Removed subscription: " << subscription << std::endl;
                }
            } else {
                std::cout << "No subscription " << subscription << " for: " << pair << std::endl;
            }
        } else {
            std::cout << "No subscription for: " << subscription << std::endl;
        }
    }

    // Получить все подписки
    const std::unordered_map<SubscriptionType, std::unordered_set<PairType>>& GetSubscriptions() const {
        return subscriptions;
    }

    // Получить все коллбеки
    const std::unordered_map<SubscriptionType, CallbackType>& GetCallbacks() const {
        return callbacks;
    }
};


class WebSession {
public:
    WebSession(int id) : id_(id), subscriptionManager(nullptr) {}
    
    int GetId() const {
        return id_;
    }
    // Установка менеджера подписок
    void SetSubscriptionManager(std::shared_ptr<SubscriptionManager> manager) {
        // Атомарно заменяем указатель на новый менеджер
        subscriptionManager.store(manager, std::memory_order_release);
    }

    // Метод для вызова всех коллбеков
    void CallCallbacks() {
        std::cout << "invoke callback for session id: " << id_ << std::endl;
        auto manager = subscriptionManager.load(std::memory_order_acquire);
        if (!manager) {
            std::cout << "SubscriptionManager is not set!" << std::endl;
            return;
        }

        const auto& callbacks = manager->GetCallbacks();
        for (const auto& [subscription, callback] : callbacks) {
            auto response = "BTCUSDT"; // Пример ответа
            callback(response);
        }
    }

private:
    int id_;  // Уникальный идентификатор сессии
    std::atomic<std::shared_ptr<SubscriptionManager>> subscriptionManager;
};

class SubscriptionChecker {
public:
    explicit SubscriptionChecker(const SubscriptionManager& manager) : manager_(manager) {}

    // Проверить, существует ли подписка для конкретного типа и пары
    bool HasSubscription(const SubscriptionManager::SubscriptionType& subscription,
                         const SubscriptionManager::PairType& pair) const {
        const auto& subscriptions = manager_.GetSubscriptions();
        auto it = subscriptions.find(subscription);
        if (it != subscriptions.end()) {
            const auto& pairs = it->second;
            return pairs.find(pair) != pairs.end();
        }
        return false;
    }

    // Проверить, есть ли подписка для конкретного типа
    bool HasSubscription(const SubscriptionManager::SubscriptionType& subscription) const {
        const auto& subscriptions = manager_.GetSubscriptions();
        return subscriptions.find(subscription) != subscriptions.end();
    }

private:
    const SubscriptionManager& manager_;
};

class SessionPool {
public:
    void AddSession(std::shared_ptr<WebSession> session) {
        sessions.push_back(session);
    }

    std::shared_ptr<WebSession> GetSession(size_t index) {
        if (index < sessions.size()) {
            return sessions[index];
        }
        return nullptr;
    }

    size_t Size() const {
        return sessions.size();
    }

private:
    std::vector<std::shared_ptr<WebSession>> sessions;
};

// Глобальный класс для распределения подписок по сессиям
class SubscriptionDistributor {
public:
    SubscriptionDistributor(SessionPool& pool) : pool_(pool) {}

    // Метод для распределения подписки по сессиям
    void SubscribeToSession(const std::string& subscription, const std::string& pair, 
                             SubscriptionManager::CallbackType callback) {
        if (pool_.Size() == 0) {
            std::cout << "No available sessions!" << std::endl;
            return;
        }

        // Используем хэш-функцию для распределения подписки
        size_t session_index = Hash(subscription, pair) % pool_.Size();
        auto session = pool_.GetSession(session_index);

        if (session) {
            std::shared_ptr<SubscriptionManager> manager = std::make_shared<SubscriptionManager>();
            session->SetSubscriptionManager(manager);
            manager->Subscribe(subscription, pair, callback);
        }
    }

private:
    SessionPool& pool_;

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

