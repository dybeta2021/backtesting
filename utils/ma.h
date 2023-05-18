//
// Created by Gavin on 2023/5/18.
//

#pragma once

namespace utils {

    class MA {
    private:
        int period_;
        std::vector<double> values_;
        double sum_;

    public:
        explicit MA(int period) : period_(period), sum_(0.0) {
            values_.reserve(period);
        }

        double update(double price) {
            if (values_.size() == period_) {
                sum_ -= values_.front();
                values_.erase(values_.begin());
            }
            values_.emplace_back(price);
            sum_ += price;
            return sum_ / static_cast<double>(values_.size());
        }

        [[nodiscard]] const std::vector<double> &get_vector() const {
            return values_;
        }
    };


    class DMA {
    private:
        double dma_{};
        std::vector<double> values_;
        size_t count_;

    public:
        DMA() {
            count_ = 0;
        }

        double update(const double &price, const double &weight) {
            count_++;
            if (count_ == 1) {
                dma_ = price;
                values_.push_back(price);
                return price;
            }

            dma_ = price * weight + (1. - weight) * dma_;
            values_.push_back(price);
            return dma_;
        }

        [[maybe_unused]] auto get_vector() {
            return values_;
        }
    };


    class EMA {
    private:
        int period_;
        size_t count_;
        double alpha_;
        double ema_;
        std::vector<double> values_;

    public:
        explicit EMA(const int &period) : period_(period) {
            alpha_ = 2.0 / (period_ + 1.);
            count_ = 0;
            ema_ = 0;
        }


        double update(const double &price) {
            const auto idx = count_++;

            if (idx == 0) {
                ema_ = price;
                values_.push_back(ema_);
                return 0.;
            }

            else if (idx < period_) {
                ema_ = alpha_ * price + (1. - alpha_) * ema_;
                values_.push_back(ema_);
                return 0.;
            } else {
                ema_ = alpha_ * price + (1. - alpha_) * ema_;
                values_.push_back(ema_);
                return ema_;
            }
        }

        [[maybe_unused]] auto get_vector() {
            return values_;
        }
    };


    class SMA {
    private:
        int period_;
        std::vector<double> values_;
        double previous_average_;
        bool is_smoothed_;

    public:
        explicit SMA(const int &period, const bool &is_smoothed = true) : period_(period), is_smoothed_(is_smoothed) {
            previous_average_ = 0.0;
        }

        double update(const double &price) {
            if (values_.size() < period_) {
                values_.push_back(price);
                return 0.0;
            } else {
                double current_average = 0.0;
                if (is_smoothed_) {
                    current_average = (previous_average_ * (period_ - 1) + price) / period_;
                } else {
                    current_average = previous_average_ + (price - values_[0]) / period_;
                }
                previous_average_ = current_average;
                values_.push_back(price);
                values_.erase(values_.begin());
                return current_average;
            }
        }

        [[maybe_unused]] auto get_vector() {
            return values_;
        }
    };


    class MACD {
    private:
        EMA ema12_;
        EMA ema26_;
        EMA ema9_;
        std::vector<double> macd_values_;

    public:
        explicit MACD(const int &short_period = 12, const int &long_period = 26, const int &signal_period = 9) : ema12_(short_period), ema26_(long_period), ema9_(signal_period) {}

        double update(const double &price) {
            double ema12 = ema12_.update(price);
            double ema26 = ema26_.update(price);
            double dif = ema12 - ema26;
            double dea = ema9_.update(dif);
            double histogram = dif - dea;
            macd_values_.push_back(histogram);
            return histogram;
        }

        [[maybe_unused]] auto get_vector() {
            return macd_values_;
        }
    };


    class KAMA {
    private:
        size_t count_;
        std::vector<double> prices_;
        std::vector<double> dma_prices_;
        std::vector<double> ama_prices_;

        EMA *ema_ptr_ = nullptr;
        DMA *dma_ptr_ = nullptr;

        double dma_;
        double ama_;
        int ama_short_;
        int ama_long_;
        int ama_num_;

    public:
        explicit KAMA(const int ama_short,
                      const int ama_long,
                      const int ama_num,
                      const int ema_num) {
            ama_short_ = ama_short;
            ama_long_ = ama_long;
            ama_num_ = ama_num;
            count_ = 0;
            dma_ = 0;
            ama_ = 0;

            ema_ptr_ = new EMA(ema_num);
            dma_ptr_ = new DMA();
        }

        ~KAMA() {
            delete ema_ptr_;
            delete dma_ptr_;
        }

        double update(const double &price) {
            const auto idx = count_++;
            prices_.push_back(price);
            if (idx < ama_num_) {
                return 0;
            }

            const auto DIR = abs(price - prices_[idx - ama_num_]);
            double VIR = 0;
            for (auto i = 1 + idx - ama_num_; i <= idx; i++) {
                VIR += abs(prices_[i] - prices_[i - 1]);
            }

            const auto ER = DIR / VIR;
            const auto FAST_SC = 2. / (ama_short_ + 1);
            const auto SLOW_SC = 2. / (ama_long_ + 1);
            const auto SSC = ER * (FAST_SC - SLOW_SC) + SLOW_SC;
            const auto CONSTANT = SSC * SSC;

            dma_ = dma_ptr_->update(price, CONSTANT);
            ama_ = ema_ptr_->update(dma_);

            dma_prices_.push_back(dma_);
            ama_prices_.push_back(ama_);
            return ama_;
        }

        auto get_vector() {
            return ama_prices_;
        }
    };
}// namespace utils