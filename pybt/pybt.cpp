//
// Created by Gavin on 2023/5/12.
//
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE//必须定义这个宏,才能输出文件名和行号
#define SPDLOG_LOG_PATTERN "[%m/%d %T.%F] [%^%=8l%$] [%6P/%-6t] [%@#%!] %v"

#include "bt/logger.h"
#include "bt/broker.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

namespace py = pybind11;
using bt::broker::Broker;
using bt::data::Bar;
using bt::data::Position;


PYBIND11_MODULE(pybt, m) {
    py::class_<Bar>(m, "Bar")
        .def(py::init<>())
        .def_readwrite("idx", &Bar::idx)
        .def_readwrite("datetime", &Bar::datetime)
        .def_readwrite("open_price", &Bar::open_price)
        .def_readwrite("high_price", &Bar::high_price)
        .def_readwrite("low_price", &Bar::low_price)
        .def_readwrite("close_price", &Bar::close_price);

    py::class_<Position>(m, "Position")
        .def(py::init<>())
        .def_readwrite("idx", &Position::idx)
        .def_readwrite("status", &Position::status)
        .def_readwrite("datetime", &Position::datetime)
        .def_readwrite("average_price", &Position::average_price)
        .def_readwrite("current_price", &Position::current_price)
        .def_readwrite("volume", &Position::volume)
        .def_readwrite("floating_pnl", &Position::floating_pnl)
        .def_readwrite("closing_pnl", &Position::closing_pnl)
        .def_readwrite("realized_pnl", &Position::realized_pnl)
        .def_readwrite("total_pnl", &Position::total_pnl);


    py::class_<bt::broker::Broker>(m, "Broker")
            .def(py::init<const std::string &, const std::string &, const bool&>())
            .def("get_size", &Broker::get_size)
            .def("get_bar", &Broker::get_bar, py::return_value_policy::reference)
            .def("get_current_bar", &Broker::get_current_bar, py::return_value_policy::reference)
            .def("get_current_position", &Broker::get_current_position, py::return_value_policy::reference)
            .def("reset", &Broker::reset)
            .def("get_start_idx", &Broker::get_start_idx)
            .def("get_end_idx", &Broker::get_end_idx)
            .def("pre_trade", &Broker::pre_trade)
            .def("insert_order", &Broker::insert_order)
            .def("post_trade", &Broker::post_trade)
            .def("save_result", &Broker::save_result)
            .def("show_position", &Broker::show_position);
}