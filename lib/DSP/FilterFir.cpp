#include "FilterFir.h"
#include "Str.h"

FilterFir::FilterFir(const std::vector<double> &taps) {
    _taps = taps;
    _size = taps.size();
    _pDelay = new double[_size];
    reset();
}

FilterFir::~FilterFir() {
    delete[] _pDelay;
}

const std::string FilterFir::toString() const {
    return String::format("FIR: %zd", _size);
}