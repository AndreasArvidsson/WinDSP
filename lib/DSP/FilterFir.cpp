#include "FilterFir.h"

FilterFir::FilterFir(const std::vector<double> &taps) {
    _taps = taps;
    _size = taps.size();
    _pDelay = new double[_size];
    reset();
}

FilterFir::~FilterFir() {
    delete[] _pDelay;
}