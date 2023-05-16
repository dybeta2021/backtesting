#include "bt/broker.h"
#include "bt/signal.h"
#include <cxxopts.hpp>
#include <vector>


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


class KAMA {
private:
    int period_;
    size_t count_;
    std::vector<double> prices_;
    std::vector<double> dma_prices_;
    std::vector<double> ama_prices_;

    EMA *ema_ptr_ = nullptr;
    DMA *dma_ptr_ = nullptr;

    double dma_;
    double ama_;

public:
    explicit KAMA(const int &period) {
        period_ = period;
        count_ = 0;
        dma_ = 0;
        ama_ = 0;

        ema_ptr_ = new EMA(3);
        dma_ptr_ = new DMA();
    }

    ~KAMA() {
        delete ema_ptr_;
        delete dma_ptr_;
    }

    double update(const double &price) {
        const auto idx = count_++;
        prices_.push_back(price);
        if (idx < period_) {
            return 0;
        }

        const auto DIR = abs(price - prices_[idx - period_]);
        double VIR = 0;
        for (auto i = 1 + idx - period_; i <= idx; i++) {
            VIR += abs(prices_[i] - prices_[i - 1]);
        }

        const auto ER = DIR / VIR;
        constexpr auto FAST_SC = 2. / (2 + 1);
        constexpr auto SLOW_SC = 2. / (30 + 1);
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


// EMA交叉
class Strategy : public bt::signal::SignalApi {
private:
    double pre_ama_{};
    double pre_signal_{};
    bool order_signal_ = false;
    KAMA *ama_ptr_ = nullptr;

public:
    explicit Strategy(const int &params=11) {
        ama_ptr_ = new KAMA(params);
    }

    ~Strategy() {
        delete ama_ptr_;
    }

    void reset(const bool &order_signal) {
        pre_signal_ = 0;
        pre_ama_ = 0;
        order_signal_ = order_signal;
    }

    double update(const bt::data::Bar &bar) override {
        const auto s1 = ama_ptr_->update(bar.close_price);
        if (pre_ama_ == 0) {
            pre_ama_ = s1;
            return 0;
        }
        const auto signal = s1 - pre_ama_;
        pre_ama_ = s1;

        double order_volume = 0;
        if (order_signal_) {
            if (signal > 0 and pre_signal_ > 0) {
                order_volume = 0;
            } else if (signal > 0 and pre_signal_ == 0) {
                order_volume = 1;
            } else if (signal > 0 and pre_signal_ < 0) {
                order_volume = 2;
            } else if (signal < 0 and pre_signal_ < 0) {
                order_volume = 0;
            } else if (signal < 0 and pre_signal_ == 0) {
                order_volume = -1;
            } else if (signal < 0 and pre_signal_ > 0) {
                order_volume = -2;
            } else {
                order_volume = 0;
            }
        }

        bt::data::Signal record{bar.datetime, bar.open_price, bar.high_price, bar.low_price, bar.close_price, signal, order_volume};
        store_.push_back(record);

        SPDLOG_DEBUG("datetime:{}, price:{}, ama:{}, signal:{}, pre_signal:{}, order_volume:{}",
                     bar.datetime, bar.close_price, s1, signal, pre_signal_, order_volume);
        pre_signal_ = signal;
        return order_volume;
    }
};


int main(int argc, char **argv) {
    cxxopts::Options options("cross_signal", "A simple trend-following indicator optimizer.");
    options.add_options()("f, filename", "Filename of SQLite", cxxopts::value<std::string>()->default_value("hs300_60min.sqlite"))("s, start_time", "Start Datetime", cxxopts::value<std::string>()->default_value("2022-01-01 09:00:00"))("e, end_time", "End Datetime", cxxopts::value<std::string>()->default_value("2023-05-11 16:00:00"))("n, params", "Params", cxxopts::value<int>()->default_value("11"));
    auto result = options.parse(argc, argv);

    auto ret = bt::utils::create_logger("clogs/test.log", "trace", false, false, false, 1, 1);
    auto broker = bt::broker::Broker(result["filename"].as<std::string>());
    broker.reset(result["start_time"].as<std::string>(), result["end_time"].as<std::string>());

    const auto params = result["params"].as<int>();
    Strategy st(params);
    st.reset(false);
    for (auto i = 0; i <  broker.get_start_idx(); i++) {
        const auto bar = broker.get_bar(i);
        const auto tmp_signal = st.update(bar);
    }

    st.reset(true);
    for (auto i = broker.get_start_idx(); i < broker.get_end_idx() + 1; i++) {
        broker.pre_trade();
        const auto bar = broker.get_current_bar();
        broker.insert_order(st.update(bar));
        broker.post_trade();
    }

    broker.show_position();
    broker.save_result("result.sqlite");
    st.save("result.sqlite");
    return 0;
}
