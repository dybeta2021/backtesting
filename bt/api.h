#pragma once
#include "logger.h"
#include "fmt/format.h"

#include "sqlite_orm/sqlite_orm.h"
#include <string>
#include <vector>

#include "data.h"
namespace bt::api {
    using bt::data::Bar;
    using bt::data::Order;
    using bt::data::Position;

    class DataApi {
    private:
        std::vector<Bar> data_{};
        size_t size_ = 0;

    public:
        explicit DataApi(const std::string &db_path, const std::string &table_name = "quote") {
            using namespace sqlite_orm;
            auto storage = make_storage(db_path,
                                        make_table(table_name,
                                                   make_column("index", &Bar::idx),
                                                   make_column("datetime", &Bar::datetime),
                                                   make_column("open", &Bar::open_price),
                                                   make_column("high", &Bar::high_price),
                                                   make_column("low", &Bar::low_price),
                                                   make_column("close", &Bar::close_price)));
            data_ = storage.get_all<Bar>();
            size_ = data_.size();
            SPDLOG_INFO("Data size:{}, start:{}, end:{}.", data_.size(), data_[0].datetime, data_[data_.size() - 1].datetime);
        }

        [[nodiscard]] auto get_size() const {
            return size_;
        }

        auto get_bar(const size_t &idx) {
            if (idx < size_) {
                return data_[idx];
            } else {
                SPDLOG_ERROR("idx range:{},{} error:{}", 0, size_, idx);
                throw std::invalid_argument("received error idx");
            }
        }
    };

    class RecordOrder {
    private:
        std::vector<Order> store_{};

    public:
        void insert_record(const Order &order) {
            store_.push_back(order);
        }

        void clear() {
            store_.clear();
        }

        void save(const std::string &db_path, const std::string &table_name = "order") {
            using namespace sqlite_orm;

            auto table = make_table(table_name,
                                    make_column("type", &Order::type),
                                    make_column("datetime", &Order::datetime),
                                    make_column("price", &Order::price),
                                    make_column("volume", &Order::volume));

            auto storage = make_storage(db_path, table);
            storage.sync_schema();
            storage.remove_all<Order>();
            const int chunksize = 5000;
            int n = store_.size();
            for (int i = 0; i < n; i += chunksize) {
                int j = std::min(i + chunksize, n);
                auto begin = store_.begin() + i;
                auto end = store_.begin() + j;
                storage.replace_range(begin, end);
            }
        }
    };


    class RecordPosition {
    private:
        std::vector<Position> store_{};

    public:
        void insert_record(const Position &pos) {
            store_.push_back(pos);
        }

        void clear() {
            store_.clear();
        }

        auto get_pnl(){
            std::vector<double> value_{};
            value_.reserve(store_.size());
            for (const auto &i: store_) {
                if (i.status == "post_trade"){
                    value_.push_back(i.total_pnl);
                }
            }
            return value_;
        }

        void save(const std::string &db_path, const std::string &table_name = "position") {
            using namespace sqlite_orm;

            auto table = make_table(table_name,
                                    make_column("status", &Position::status),
                                    make_column("datetime", &Position::datetime),
                                    make_column("average_price", &Position::average_price),
                                    make_column("current_price", &Position::current_price),
                                    make_column("volume", &Position::volume),
                                    make_column("floating_pnl", &Position::floating_pnl),
                                    make_column("closing_pnl", &Position::closing_pnl),
                                    make_column("realized_pnl", &Position::realized_pnl),
                                    make_column("total_pnl", &Position::total_pnl));

            auto storage = make_storage(db_path, table);
            storage.sync_schema();
            storage.remove_all<Position>();
            const int chunksize = 5000;
            int n = store_.size();
            for (int i = 0; i < n; i += chunksize) {
                int j = std::min(i + chunksize, n);
                auto begin = store_.begin() + i;
                auto end = store_.begin() + j;
                storage.replace_range(begin, end);
            }
        }
    };

}// namespace bt::api
