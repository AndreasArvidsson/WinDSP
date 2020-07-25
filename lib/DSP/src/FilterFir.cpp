#include "FilterFir.h"
#include "Str.h"

using std::make_unique;

FilterFir::FilterFir(const vector<double>& taps) {
    _taps = taps;
    _size = taps.size();
    _pDelay = make_unique<double[]>(_size);
    reset();
}

const vector<string> FilterFir::toString() const {
    return vector<string>{
        String::format("FIR: %zdtaps", _size)
    };
}