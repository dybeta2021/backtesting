#pragma once
#include "api.h"
#include "data.h"
#include "logger.h"
#include "spdlog/spdlog.h"

namespace bt::broker {
    using bt::data::Bar;
    using bt::data::Order;
    using bt::data::Position;

    class Broker {
    private:
        bt::api::DataApi *data_api_ = nullptr;
        bt::api::RecordOrder *record_order_api_ = nullptr;
        bt::api::RecordPosition *record_position_api_ = nullptr;

        size_t current_idx_ = 0;
        size_t start_idx_ = 0;
        size_t end_idx_ = 0;
        size_t order_idx_ = 0;
        Position position_{};

    private:
        /// order>0, pos>=0
        int long_open(const bt::api::Bar &bar, const double &volume) {
            if (volume <= 0) {
                SPDLOG_ERROR("error order volume:{}", volume);
                return -1;
            }

            if (position_.volume < 0) {
                SPDLOG_ERROR("error position volume:{}", volume);
                return -1;
            }

            // recording
            auto order = Order{};
            order.idx = order_idx_++;
            order.type = "long_open";
            order.datetime = bar.datetime;
            order.price = bar.close_price;
            order.volume = volume;
            record_order_api_->insert_record(order);
            SPDLOG_DEBUG("long_open, datetime:{}, price:{}, volume:{}", order.datetime, order.price, order.volume);

            position_.average_price = (position_.average_price * position_.volume + order.price * order.volume) / (position_.volume + order.volume);
            position_.volume += order.volume;
            return 0;
        }

        /// order<0, pos>0
        int long_close(const bt::api::Bar &bar, const double &order_volume) {
            if (order_volume >= 0) {
                SPDLOG_ERROR("error order volume:{}", order_volume);
                return -1;
            }

            if (position_.volume <= 0) {
                SPDLOG_ERROR("error position volume:{}", order_volume);
                return -1;
            }

            if (-order_volume > position_.volume) {
                SPDLOG_ERROR("error order volume:{}", order_volume);
                return -1;
            }

            // recording
            auto order = Order{};
            order.idx = order_idx_++;
            order.type = "long_close";
            order.datetime = bar.datetime;
            order.price = bar.close_price;
            order.volume = order_volume;
            record_order_api_->insert_record(order);
            SPDLOG_DEBUG("long_close, datetime:{}, price:{}, volume:{}", order.datetime, order.price, order.volume);


            // 全平
            if (position_.volume + order.volume == 0) {
                position_.closing_pnl = (order.price - position_.average_price) * position_.volume;
                position_.volume = 0;
                position_.average_price = 0;
                position_.floating_pnl = 0;
            } else {
                position_.closing_pnl = -order.price * order.volume + position_.average_price * order.volume;
                position_.volume += order.volume;
                position_.floating_pnl = (position_.current_price - position_.average_price) * position_.volume;
            }
            return 0;
        }


        /// order<0, pos<=0
        int short_open(const bt::api::Bar &bar, const double &volume) {
            if (volume >= 0) {
                SPDLOG_ERROR("error order volume:{}", volume);
                return -1;
            }

            if (position_.volume > 0) {
                SPDLOG_ERROR("error position volume:{}", volume);
                return -1;
            }

            // recording
            auto order = Order{};
            order.idx = order_idx_++;
            order.type = "short_open";
            order.datetime = bar.datetime;
            order.price = bar.close_price;
            order.volume = volume;
            record_order_api_->insert_record(order);
            SPDLOG_DEBUG("short_open, datetime:{}, price:{}, volume:{}", order.datetime, order.price, order.volume);


            position_.average_price = (position_.average_price * position_.volume + order.price * order.volume) / (position_.volume + order.volume);
            position_.volume += order.volume;
            return 0;
        }


        /// order>0, pos<0
        int short_close(const bt::api::Bar &bar, const double &order_volume) {
            if (order_volume <= 0) {
                SPDLOG_ERROR("error order volume:{}", order_volume);
                return -1;
            }

            if (position_.volume >= 0) {
                SPDLOG_ERROR("error position volume:{}", order_volume);
                return -1;
            }

            if (order_volume > -position_.volume) {
                SPDLOG_ERROR("error order volume:{}", order_volume);
                return -1;
            }

            // recording
            auto order = Order{};
            order.idx = order_idx_++;
            order.type = "short_close";
            order.datetime = bar.datetime;
            order.price = bar.close_price;
            order.volume = order_volume;
            record_order_api_->insert_record(order);
            SPDLOG_DEBUG("short_close, datetime:{}, price:{}, volume:{}", order.datetime, order.price, order.volume);

            // 全平
            if (position_.volume + order.volume == 0) {
                position_.closing_pnl = -position_.average_price * position_.volume - order.price * order.volume;
                position_.volume = 0;
                position_.average_price = 0;
                position_.floating_pnl = 0;
            } else {
                position_.closing_pnl = -order.price * order.volume + position_.average_price * order.volume;
                position_.volume += order.volume;
                position_.floating_pnl = (position_.current_price - position_.average_price) * position_.volume;
            }
            return 0;
        }


    public:
        explicit Broker(const std::vector<Bar> &data) {
            data_api_ = new bt::api::DataApi(data);
            record_order_api_ = new bt::api::RecordOrder();
            record_position_api_ = new bt::api::RecordPosition();
        }

        ~Broker() {
            delete data_api_;
            delete record_order_api_;
            delete record_position_api_;
        }

        auto get_size() {
            return data_api_->get_size();
        }

        auto get_bar(const size_t &idx) {
            return data_api_->get_bar(idx);
        }

        auto get_pre_bar() {
            if (current_idx_ == 0) {
                return bt::data::Bar{};
            }
            return data_api_->get_bar(current_idx_);
        }

        auto get_current_bar() {
            return data_api_->get_bar(current_idx_);
        }

        auto get_current_position() {
            return position_;
        }

        auto reset(const std::string &start_time, const std::string &end_time) {
            position_ = {};
            current_idx_ = 0;
            start_idx_ = 0;
            end_idx_ = 0;
            order_idx_ = 0;

            for (auto i = 0; i < data_api_->get_size(); i++) {
                const auto &bar = data_api_->get_bar(i);
                if (start_time < bar.datetime) {
                    start_idx_ = i;
                    break;
                }

                else if (end_time < bar.datetime) {
                    start_idx_ = i - 1;
                    break;
                }
            }

            for (auto i = 0; i < data_api_->get_size(); i++) {
                const auto &bar = data_api_->get_bar(i);
                if (end_time < bar.datetime) {
                    end_idx_ = i - 1;
                    break;
                }

                else if (end_time == bar.datetime) {
                    end_idx_ = i;
                    break;
                }
            }

            if (end_idx_ == 0) {
                end_idx_ = get_size() - 1;
                SPDLOG_WARN("error params:{}, {}, data range:{}, {}", start_time, end_time, get_bar(0).datetime, get_bar(get_size() - 1).datetime);
            }

            if (start_idx_ >= end_idx_) {
                SPDLOG_ERROR("error params:{}, {}", start_time, end_time);
                return -1;
            }
            current_idx_ = start_idx_;
            SPDLOG_INFO("Reset, start_time:{}, end_time:{}, current_idx:{}.", data_api_->get_bar(start_idx_).datetime, data_api_->get_bar(end_idx_).datetime, current_idx_);
            return 0;
        }

        [[nodiscard]] auto get_start_idx() const {
            return start_idx_;
        }

        [[nodiscard]] auto get_end_idx() const {
            return end_idx_;
        }

        void pre_trade() {
            const auto bar = data_api_->get_bar(current_idx_);
            position_.idx = current_idx_;
            position_.datetime = bar.datetime;
            position_.current_price = bar.close_price;
            position_.status = "pre_trade";
            if (position_.volume != 0) {
                position_.floating_pnl = (position_.current_price - position_.average_price) * position_.volume;
            }
            position_.total_pnl = position_.realized_pnl + position_.floating_pnl;
            record_position_api_->insert_record(position_);
        }

        void insert_order(double volume) {
            if (volume == 0) {
                return;
            }
            position_.status = "trade";

            const auto bar = data_api_->get_bar(current_idx_);
            const auto current_pos = position_.volume;

            if (current_pos == 0) {
                if (volume > 0) {
                    long_open(bar, volume);
                } else {
                    short_open(bar, volume);
                }
            } else if (current_pos > 0) {
                if (volume > 0) {
                    long_open(bar, volume);
                } else {
                    auto left_volume = current_pos + volume;
                    if (left_volume >= 0) {
                        long_close(bar, volume);
                    } else {
                        long_close(bar, -current_pos);
                        short_open(bar, left_volume);
                    }
                }
            } else {
                if (volume < 0) {
                    short_open(bar, volume);
                } else {
                    auto left_volume = current_pos + volume;
                    if (left_volume <= 0) {
                        short_close(bar, volume);
                    } else {
                        short_close(bar, -current_pos);
                        long_open(bar, left_volume);
                    }
                }
            }
        }

        void post_trade() {
            auto bar = data_api_->get_bar(current_idx_++);
            position_.status = "post_trade";
            position_.current_price = bar.close_price;
            position_.floating_pnl = (position_.current_price - position_.average_price) * position_.volume;
            position_.realized_pnl += position_.closing_pnl;
            position_.closing_pnl = 0;
            position_.total_pnl = position_.realized_pnl + position_.floating_pnl;
            record_position_api_->insert_record(position_);
        }

        const auto get_record_api() {
            return record_position_api_;
        }

        void save_result(const std::string &db_path) {
            record_order_api_->save(db_path);
            record_position_api_->save(db_path);
        }


    public:
        void show_position() {
            SPDLOG_INFO("idx:{}, status:{}, datetime:{}, total_pnl:{}, realized_pnl:{},  closing_pnl:{}, floating_pnl:{}, "
                        "average_price:{}, current_price:{}, volume:{} ",
                        position_.idx,
                        position_.status,
                        position_.datetime,
                        position_.total_pnl,
                        position_.realized_pnl,
                        position_.closing_pnl,
                        position_.floating_pnl,
                        position_.average_price,
                        position_.current_price,
                        position_.volume);
        }
    };
}// namespace bt::broker