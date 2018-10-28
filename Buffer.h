#pragma once

struct Buffer {
	double *_pData;
	size_t _size, _capacity, _numChannels, _totalByteSize;
	std::mutex _dataMutex, _sizeMutex;

	Buffer() {
		_pData = nullptr;
		_size = _capacity = _numChannels = _totalByteSize = 0;
	}

	void init(const size_t capacity, const size_t numChannels) {
		_capacity = capacity;
		_numChannels = numChannels;
		_pData = new double[capacity * numChannels];
		_totalByteSize = capacity * numChannels * sizeof(double);
		memset(_pData, 0, _totalByteSize);
		_size = 0;
	}

	void destroy() {
		delete[] _pData;
		_pData = nullptr;
	}

	inline const size_t getAvailableSize() {
		std::lock_guard<std::mutex> lock(_sizeMutex);
		return _capacity - _size;
	}

	inline const size_t size() {
		std::lock_guard<std::mutex> lock(_sizeMutex);
		return _size;
	}

	inline double* getData() {
		_dataMutex.lock();
		return _pData + size() * _numChannels;
	}

	inline void releaseData(const size_t length) {
		_dataMutex.unlock();
		if (size() + length == _capacity) {
			resetSize();
		}
		else {
			setSize(size() + length);
		}
	}

	inline void releaseData() {
		_dataMutex.unlock();
	}

	inline void resetSize() {
		setSize(0);
	}

	inline void fillWithSilence() {
		memset(getData(), 0, _totalByteSize);
		releaseData();
		setSize(_capacity);
	};

	inline void setSize(const size_t size) {
		_sizeMutex.lock();
		_size = size;
		_sizeMutex.unlock();
	}

};