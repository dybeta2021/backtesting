#pragma once

namespace bt::signal {
    using bt::data::Signal;
    class SignalApi {
    protected:
        std::vector<Signal> store_{};

    public:
        virtual double update(const bt::data::Bar &bar) {
            return 0;
        };

        void save(const std::string &db_path, const std::string &table_name = "signal_table") {
            using namespace sqlite_orm;
            auto table = make_table(table_name,
                                    make_column("datetime", &Signal::datetime),
                                    make_column("open_price", &Signal::open_price),
                                    make_column("high_price", &Signal::high_price),
                                    make_column("low_price", &Signal::low_price),
                                    make_column("close_price", &Signal::close_price),
                                    make_column("signal", &Signal::signal),
                                    make_column("order_volume", &Signal::order_volume));

            auto storage = make_storage(db_path, table);
            storage.sync_schema();
            storage.remove_all<Signal>();
            const int chunksize = 1000;
            int n = store_.size();
            for (int i = 0; i < n; i += chunksize) {
                int j = std::min(i + chunksize, n);
                auto begin = store_.begin() + i;
                auto end = store_.begin() + j;
                storage.replace_range(begin, end);
            }

        }

        auto get_signal() {
            std::vector<double> signal_{};
            signal_.reserve(store_.size());
            for (const auto &i: store_) {
                signal_.push_back(i.signal);
            }
            return signal_;
        }

        auto get_price() {
            std::vector<double> price_{};
            price_.reserve(store_.size());
            for (const auto &i: store_) {
                price_.push_back(i.close_price);
            }
            return price_;
        }

        auto get_order_volume() {
            std::vector<double> order_volume_{};
            order_volume_.reserve(store_.size());
            for (const auto &i: store_) {
                order_volume_.push_back(i.order_volume);
            }
            return order_volume_;
        }
    };
}// namespace bt::signal