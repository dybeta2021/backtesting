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

        void save(const std::string &db_path, const std::string &table_name = "signal") {
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
            storage.insert_range(store_.begin(), store_.end());
        }
    };
}// namespace bt::signal