#pragma once
#include <atomic>

#include <mutex>

class Buffer {
public:

	void lock() {
		std::lock_guard<std::mutex> guard(mutex);
		if (isLocked) {
			printf("Already locked!\n");
		}
		isLocked = true;
	}

	void unlock() {
		std::lock_guard<std::mutex> guard(mutex);
		if (!isLocked) {
			printf("Not locked!\n");
		}
		isLocked = false;
	}

	Buffer() {
		_pData = nullptr;
		_capacity = _numChannels = _totalByteSize = 0;
		_size = 0;
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
		return _capacity - _size;
	}

	inline const size_t size() {
		return _size;
	}

	inline const size_t capacity() {
		return _capacity;
	}

	inline const size_t numChannels() {
		return _numChannels;
	}

	inline double* getData() {
		_dataMutex.lock();
		return _pData;
	}

	inline double* getDataWithOffset() {
		_dataMutex.lock();
		return _pData + _size * _numChannels;
	}

	inline void releaseData(const size_t length) {
		_dataMutex.unlock();
		_size += length;
#ifdef DEBUG
		if (_size > _capacity) {
			printf("Buffeer overflow: Size: %zd, Capacity: %zd\n", _size.load(), _capacity);
		}
#endif
	}

	inline void releaseData() {
		_dataMutex.unlock();
		_size = 0;
	}

	inline void resetSize() {
		_size = 0;
	}

	inline void fillWithSilence() {
		memset(getData(), 0, _totalByteSize);
		releaseData();
		_size = _capacity;
	};

	inline const bool isFull() const {
		return _size == _capacity;
	}

private:
	std::atomic<size_t> _size;
	size_t _capacity, _numChannels, _totalByteSize;
	double *_pData;
	std::mutex _dataMutex;


	std::mutex mutex;
	bool isLocked = false;


};