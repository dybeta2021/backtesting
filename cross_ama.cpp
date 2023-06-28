#include "bt/interface.h"
#include "ma.h"
#include <string>
#include <vector>

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
    for (auto i = 0; i < broker.get_start_idx(); i++) {
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
