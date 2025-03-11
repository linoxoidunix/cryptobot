#include <cmath>
#include <deque>
#include <iostream>
#include <vector>

template <class T>
class SlidingWindow {
    std::deque<T> window_;
    T actual_sum_ = 0;
    size_t window_size_;
    T last_value_;
    bool last_value_was_init = false;

  public:
    SlidingWindow(size_t window_size) : window_size_(window_size) {}

    void AddElement(T t) {
        last_value_was_init  = true;
        actual_sum_         += t;
        window_.push_back(t);
        last_value_ = t;
        if (window_.size() > window_size_) {
            actual_sum_ -= window_.front();
            window_.pop_front();
        }
    }

    std::pair<bool, T> GetAverage() const {
        if (window_.size() < window_size_) return {false, 0};
        return {true, actual_sum_ / window_size_};
    }

    std::pair<bool, T> LastValue() const {
        return {last_value_was_init, last_value_};
    }
};

struct SharpMove {
    size_t index;
    double price_change;
    double avg_change;
};

std::vector<SharpMove> detectSharpMoves(const std::vector<double>& prices,
                                        size_t window_size, double threshold) {
    std::vector<SharpMove> sharp_moves;
    SlidingWindow<double> change_window(window_size);

    for (size_t i = 1; i < prices.size(); ++i) {
        double current_change =
            prices[i] - prices[i - 1];  // Сохраняем знак изменения
        change_window.AddElement(
            std::abs(current_change));  // Среднее по модулю

        auto [valid, avg_change] = change_window.GetAverage();
        if (valid && std::abs(current_change) > threshold * avg_change) {
            sharp_moves.push_back({i, current_change, avg_change});
        }
    }

    return sharp_moves;
}

// int main() {
//     // Пример данных (цены Binance)
//     std::vector<double> binance_prices = {
//         50000.0, 50020.5, 50010.2, 49980.3, 50005.7, 50050.1, 50030.8,
//         49990.6, 49970.2, 50015.0, 50040.7, 50035.9, 50025.3, 50010.5,
//         49995.2, 50005.8, 50030.4, 50055.1, 50060.8, 50045.6, 50035.9,
//         50020.3, 49990.7, 49985.2, 49970.6, 49995.8, 50015.2, 50035.7,
//         50050.9, 50080.3, 50100.5, 50095.2};

//     size_t window_size = 5;    // Размер окна для среднего изменения
//     double threshold   = 1.2;  // Пороговое значение (2x от среднего)

//     std::vector<SharpMove> spikes =
//         detectSharpMoves(binance_prices, window_size, threshold);

//     std::cout << "Резкие изменения цены:\n";
//     for (const auto& spike : spikes) {
//         std::cout << "Index: " << spike.index
//                   << " | Price Change: " << spike.price_change
//                   << " | Avg Change: " << spike.avg_change
//                   << " | Ratio: " << spike.price_change / spike.avg_change
//                   << "x\n";
//     }

//     return 0;
// }

#include <algorithm>
#include <cmath>
#include <deque>
#include <iostream>
#include <map>
#include <memory>

// Интерфейс для вычисления гистограммы
class IHistogramCalculator {
  public:
    virtual ~IHistogramCalculator() = default;
    virtual std::map<int, double> ComputeHistogram(
        const std::deque<double>& data, int bins) = 0;
};

// Интерфейс для вычисления совместной гистограммы
class IJointHistogramCalculator {
  public:
    virtual ~IJointHistogramCalculator() = default;
    virtual std::map<std::pair<int, int>, double> ComputeJointHistogram(
        const std::deque<double>& x, const std::deque<double>& y, int bins) = 0;
};

// Интерфейс для вычисления взаимной информации
class IMutualInformationCalculator {
  public:
    virtual ~IMutualInformationCalculator()           = default;
    virtual double ComputeMutualInformation(const std::deque<double>& x,
                                            const std::deque<double>& y,
                                            int bins) = 0;
};

// Стратегия для вычисления гистограммы
class HistogramCalculator : public IHistogramCalculator {
  public:
    std::map<int, double> ComputeHistogram(const std::deque<double>& data,
                                           int bins) override {
        std::map<int, double> histogram;
        double min_val   = *std::min_element(data.begin(), data.end());
        double max_val   = *std::max_element(data.begin(), data.end());
        double bin_width = (max_val - min_val) / bins;

        for (double value : data) {
            int bin_index = (value - min_val) / bin_width;
            histogram[bin_index]++;
        }

        // Нормируем на общее число элементов
        for (auto& kv : histogram) {
            kv.second /= data.size();
        }

        return histogram;
    }
};

// Стратегия для вычисления совместной гистограммы
class JointHistogramCalculator : public IJointHistogramCalculator {
  public:
    std::map<std::pair<int, int>, double> ComputeJointHistogram(
        const std::deque<double>& x, const std::deque<double>& y,
        int bins) override {
        std::map<std::pair<int, int>, double> joint_histogram;
        double min_x       = *std::min_element(x.begin(), x.end());
        double max_x       = *std::max_element(x.begin(), x.end());
        double min_y       = *std::min_element(y.begin(), y.end());
        double max_y       = *std::max_element(y.begin(), y.end());

        double bin_width_x = (max_x - min_x) / bins;
        double bin_width_y = (max_y - min_y) / bins;

        for (size_t i = 0; i < x.size(); ++i) {
            int bin_x = (x[i] - min_x) / bin_width_x;
            int bin_y = (y[i] - min_y) / bin_width_y;
            joint_histogram[{bin_x, bin_y}]++;
        }

        // Нормируем на общее число элементов
        for (auto& kv : joint_histogram) {
            kv.second /= x.size();
        }

        return joint_histogram;
    }
};

// Стратегия для вычисления взаимной информации
class MutualInformationCalculator : public IMutualInformationCalculator {
  public:
    MutualInformationCalculator(
        std::shared_ptr<IHistogramCalculator> hist_calculator,
        std::shared_ptr<IJointHistogramCalculator> joint_hist_calculator)
        : hist_calculator_(std::move(hist_calculator)),
          joint_hist_calculator_(std::move(joint_hist_calculator)) {}

    double ComputeMutualInformation(const std::deque<double>& x,
                                    const std::deque<double>& y,
                                    int bins) override {
        auto hist_x = hist_calculator_->ComputeHistogram(x, bins);
        auto hist_y = hist_calculator_->ComputeHistogram(y, bins);
        auto joint_hist =
            joint_hist_calculator_->ComputeJointHistogram(x, y, bins);

        double mi = 0.0;
        for (const auto& kv : joint_hist) {
            int bin_x   = kv.first.first;
            int bin_y   = kv.first.second;
            double p_xy = kv.second;
            double p_x  = hist_x[bin_x];
            double p_y  = hist_y[bin_y];

            if (p_xy > 0 && p_x > 0 && p_y > 0) {
                mi += p_xy * std::log2(p_xy / (p_x * p_y));
            }
        }

        return mi;
    }

  private:
    std::shared_ptr<IHistogramCalculator> hist_calculator_;
    std::shared_ptr<IJointHistogramCalculator> joint_hist_calculator_;
};

class RealTimeMutualInformation {
  public:
    RealTimeMutualInformation(
        int window_size, int bins,
        std::shared_ptr<IMutualInformationCalculator> mi_calculator)
        : window_size_(window_size),
          bins_(bins),
          mi_calculator_(std::move(mi_calculator)) {}

    void AddDataPoint(double x, double y) {
        x_window_.push_back(x);
        y_window_.push_back(y);

        if (x_window_.size() > window_size_) {
            x_window_.pop_front();
            y_window_.pop_front();
        }

        if (x_window_.size() == window_size_) {
            double mi = mi_calculator_->ComputeMutualInformation(
                x_window_, y_window_, bins_);
            std::cout << "Mutual Information (current window): " << mi
                      << std::endl;
        }
    }

  private:
    int window_size_;
    int bins_;
    std::deque<double> x_window_;
    std::deque<double> y_window_;
    std::shared_ptr<IMutualInformationCalculator> mi_calculator_;
};

// int main() {
//     auto hist_calculator       = std::make_shared<HistogramCalculator>();
//     auto joint_hist_calculator =
//     std::make_shared<JointHistogramCalculator>(); auto mi_calculator =
//     std::make_shared<MutualInformationCalculator>(
//         hist_calculator, joint_hist_calculator);

//     RealTimeMutualInformation rtmi(5, 10, mi_calculator);

//     int num_points = 100;  // Генерируем 100 точек данных
//     // auto incoming_data = GenerateNonLinearData(num_points);

//     // Пример поступающих данных в реальном времени
//     // std::vector<std::pair<double, double>> incoming_data = {
//     //     {1.2, 2.3}, {3.4, 3.1}, {2.2, 1.9}, {4.5, 4.2}, {5.1, 5.3},
//     //     {3.3, 3.5}, {2.8, 2.7}, {4.1, 4.3}, {5.6, 5.7}, {3.9, 3.8}};

//     std::vector<std::pair<double, double>> incoming_data = {
//         {29300.25, 29290.10}, {29350.50, 29340.35}, {29400.75, 29380.20},
//         {29450.00, 29430.15}, {29500.25, 29488.05}, {29550.50, 29520.40},
//         {29602.75, 29560.60}, {29000.00, 29600.50}, {49700.25, 29650.40},
//         {29750.50, 59700.35}};

//     // Обрабатываем данные по одному
//     for (const auto& data : incoming_data) {
//         rtmi.AddDataPoint(data.first, data.second);
//     }

//     return 0;
// }

#include "aot/strategy/arbitrage/step_manager.h"
int main() {
    aot::StepManager step_manager(5);

    return 0;
}
