#pragma once
#include <sciplot/sciplot.hpp>
#include "fmt/format.h"
#include <numeric>
#include <iostream>
#include <vector>
#include "sciplot/sciplot.hpp"

namespace bt::plot {
    int plot_pnl() {
        using namespace sciplot;

        // Generate some sample data
        std::vector<double> pnl = {0.1, 0.2, 0.3, 0.2, 0.1, -0.1, -0.2, -0.1};
        std::vector<double> price = {100, 101, 102, 103, 102, 101, 100, 99};
        std::vector<double> order_volume = {0, 0, 0, 0, 100, -50, 0, 0};

        std::vector<size_t> idx;
        idx.resize(pnl.size());
        std::iota(idx.begin(), idx.end(), 0);

        // Create a plot object with two subplots
        Plot2D plot1;
        plot1.xlabel("idx");
        plot1.ylabel("pnl");
        plot1.drawCurve(idx, pnl).label("pnl");

        Plot2D plot2;
        plot2.xlabel("idx");
        plot2.ylabel("price");
        plot2.drawCurve(idx, price).label("price");

        for (size_t i = 0; i < order_volume.size(); i++) {
            if (order_volume[i] > 0) {
                const std::string sh = fmt::format("set arrow from graph {0},{1} to graph {0},{2} filled back lw 3 lc rgb \"red\"", idx[i], price[i]*0.99, price[i]);
                plot2.gnuplot(sh);
            }
            // If the order_volume is negative, draw a downward green arrow at the corresponding price
            else if (order_volume[i] < 0) {
                const std::string sh = fmt::format("set arrow from graph {0},{1} to graph {0},{2} filled back lw 3 lc rgb \"red\"", idx[i], price[i], price[i]*0.99);
                plot2.gnuplot(sh);
            }
        }



        // Create figure to hold plot
        Figure fig = {{plot1}, {plot2}};
        // Create canvas to hold figure
        Canvas canvas = {{fig}};
        canvas.size(1200, 600);
        // Show the plot in a pop-up window
        canvas.show();
        return 0;
    }

}


