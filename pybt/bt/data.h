//
// Created by Gavin on 2023/5/11.
//

#pragma once

#include <string>

namespace bt::data{
    struct Bar {
        size_t idx{};
        std::string datetime;
        double open_price{};
        double high_price{};
        double low_price{};
        double close_price{};
    };

    struct Signal {
        std::string datetime;
        double open_price{};
        double high_price{};
        double low_price{};
        double close_price{};
        double signal;
        double order_volume;
    };


    struct Order {
        size_t idx{};
        std::string type;
        std::string datetime;
        double price;
        double volume;
    };

    struct Position {
        size_t idx{};
        std::string status;
        std::string datetime;
        double average_price;
        double current_price;
        double volume;
        double floating_pnl;//持仓盈亏
        double closing_pnl; //平仓盈亏
        double realized_pnl;//已实现盈亏
        double total_pnl;
    };


}