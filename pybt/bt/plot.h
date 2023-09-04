#pragma once
#include "fmt/format.h"
#include "sciplot/sciplot.hpp"
#include <iostream>
#include <numeric>
#include <vector>

namespace bt::plot {
    int plot_pnl(const std::vector<double> &pnl, const std::vector<double> &price) {
        using namespace sciplot;
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

        // Create figure to hold plot
        Figure fig = {{plot1}, {plot2}};
        // Create canvas to hold figure
        Canvas canvas = {{fig}};
        canvas.size(1200, 600);
        // Show the plot in a pop-up window
        canvas.show();
        return 0;
    }

}// namespace bt::plot
