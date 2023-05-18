//
// Created by 稻草人 on 2023/5/17.
//
#include "bt/interface.h"
#include "utils/ma.h"
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex mutex;// 用于保护params_list的互斥量
using utils::KAMA;


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

struct Params {
    double ama_short;
    double ama_long;
    double ama_num;
    double ema_num;
    double final_pnl;
};


auto calc_instance(const std::vector<bt::data::Bar> &data,
                   const std::string &start_time,
                   const std::string &end_time,
                   const int &ama_short,
                   const int &ama_long,
                   const int &ama_num,
                   const int &ema_num) {
    auto broker = bt::broker::Broker(data);
    broker.reset(start_time, end_time);
    Strategy st(ama_short, ama_long, ama_num, ema_num);
    st.reset(false);
    for (auto i = 0; i < broker.get_start_idx(); i++) {
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

    auto position = broker.get_record_api();
    const auto price_list = position->get_price();
    const auto pnl_list = position->get_pnl();
    const auto datetime_list = position->get_datetime();
    const auto position_list = position->get_position();
    const auto final_pnl = pnl_list[pnl_list.size() - 1];

    Params params{};
    params.ama_short = ama_short;
    params.ama_long = ama_long;
    params.ama_num = ama_num;
    params.ema_num = ema_num;
    params.final_pnl = final_pnl;
    return params;
}


void calc_and_push(std::vector<Params> &params_list,
                   const std::vector<bt::data::Bar> &data,
                   const std::string &start_time,
                   const std::string &end_time,
                   const int ama_long,
                   const int ama_short,
                   const int ama_num,
                   const int ema_num) {
    SPDLOG_INFO("ama_short:{}, ama_num:{}, ema_num:{}", ama_short, ama_num, ema_num);
    const auto params = calc_instance(data, start_time, end_time, ama_short, ama_long, ama_num, ema_num);
    std::lock_guard<std::mutex> lock(mutex);
    params_list.push_back(params);
}


int main(int argc, char *argv[]) {
    std::cout << "argc = " << argc << std::endl;
    for (int i = 0; i < argc; i++) {
        std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
    }
    const std::string db_name = argv[1];
    const auto data = bt::api::load_db(db_name);
    const std::string start_time = argv[2];
    const std::string end_time = argv[3];

    auto ret = bt::utils::create_logger("clogs/test.log", "info", false, false, false, 1, 1);

    std::vector<Params> params_list{};
    const auto ama_long = 30;
    std::vector<std::thread> threads;
    for (auto ama_short: {1, 2, 3, 4, 5, 6}) {
        for (auto ama_num = 2; ama_num < 41; ama_num++) {
            for (auto ema_num = 1; ema_num < 21; ema_num++) {
                threads.emplace_back(calc_and_push, std::ref(params_list), data, start_time, end_time, ama_long, ama_short, ama_num, ema_num);
            }
        }
    }

    for (auto &thread: threads) {
        thread.join();
    }
    SPDLOG_INFO("thread finish.");

    {
        using namespace sqlite_orm;
        auto table = make_table("params",
                                make_column("ama_short", &Params::ama_short),
                                make_column("ama_long", &Params::ama_long),
                                make_column("ama_num", &Params::ama_num),
                                make_column("ema_num", &Params::ema_num),
                                make_column("final_pnl", &Params::final_pnl));

        auto storage = make_storage("result.sqlite", table);
        storage.sync_schema();
        storage.remove_all<Params>();

        const int chunksize = 5000;
        int n = params_list.size();
        for (int i = 0; i < n; i += chunksize) {
            int j = std::min(i + chunksize, n);
            auto begin = params_list.begin() + i;
            auto end = params_list.begin() + j;
            storage.replace_range(begin, end);
        }
    }

    return 0;
}
