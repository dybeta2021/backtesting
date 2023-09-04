#include "bt/interface.h"
#include <string>
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


// EMA交叉
class Strategy : public bt::signal::SignalApi {
private:
    double pre_ama_{};
    double pre_signal_{};
    bool order_signal_ = false;
    KAMA *ama_ptr_ = nullptr;

public:
    explicit Strategy(const int ama_short,
                      const int ama_long,
                      const int ama_num,
                      const int ema_num) {
        ama_ptr_ = new KAMA(ama_short, ama_long, ama_num, ema_num);
    }

    ~Strategy() {
        delete ama_ptr_;
    }

    void reset(const bool &order_signal) {
        pre_signal_ = 0;
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

int main(int argc, char *argv[]) {
    std::cout << "argc = " << argc << std::endl;
    for (int i = 0; i < argc; i++) {
        std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
    }
    const std::string db_name = argv[1];
    const auto data = bt::api::load_db(db_name);
    const std::string start_time = argv[2];
    const std::string end_time = argv[3];
    const int ama_short = std::stoi(argv[4]);
    const int ama_long = std::stoi(argv[5]);
    const int ama_num = std::stoi(argv[6]);
    const int ema_num = std::stoi(argv[7]);


    auto ret = bt::utils::create_logger("clogs/test.log", "info", false, false, false, 1, 1);
    auto broker = bt::broker::Broker(data);
    broker.reset(start_time, end_time);

    Strategy st(ama_short, ama_long, ama_num, ema_num);
    st.reset(false);
    for (auto i = 0; i <  broker.get_start_idx(); i++) {
        const auto bar = broker.get_bar(i);
        const auto tmp_signal = st.update(bar);
        SPDLOG_INFO("idx:{}, datetime:{}, price:{}, signal:{}", bar.idx, bar.datetime, bar.close_price, tmp_signal);
    }

    const auto bar = broker.get_bar(broker.get_start_idx());
    SPDLOG_INFO("start_idx:{}, idx:{}, datetime:{}", broker.get_start_idx(), bar.idx, bar.datetime);


    st.reset(true);
    for (auto i = broker.get_start_idx(); i < broker.get_end_idx() + 1; i++) {
        broker.pre_trade();
        const auto bar = broker.get_current_bar();
        const auto tmp_signal = st.update(bar);
        SPDLOG_INFO("idx:{}, datetime:{}, price:{}, signal:{}", bar.idx, bar.datetime, bar.close_price, tmp_signal);
        broker.insert_order(tmp_signal);
        broker.post_trade();
    }

    broker.show_position();
    broker.save_result("value_result.sqlite");
    st.save("value_result.sqlite");

    const auto pnl = broker.get_record_api()->get_pnl();
    const auto price = st.get_price();
    broker.show_position();
    return 0;
}




