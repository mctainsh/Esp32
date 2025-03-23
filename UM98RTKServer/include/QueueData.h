#ifndef QUEUE_DATA_H
#define QUEUE_DATA_H

#include <cstddef>
#include <cstring>
#include <Arduino.h>

class QueueData
{
private:
	unsigned char *_pData;
	size_t _length;
	unsigned long _timestamp;

public:
	////////////////////////////////////////
	// Constructor
	QueueData(const unsigned char *inputData, size_t inputLength)
		: _pData(nullptr), _length(inputLength)
	{
		if (inputData && inputLength > 0)
		{
			_pData = new unsigned char[inputLength];
			std::memcpy(_pData, inputData, inputLength);
		}
		_timestamp = millis();
	}

	////////////////////////////////////////
	// Destructor
	~QueueData()
	{
		if( _pData != nullptr )
			delete[] _pData;
	}

	////////////////////////////////////////
	// Check if the data is expired
	bool IsExpired(unsigned long timeoutMs) const
	{
		return (millis() - _timestamp) > timeoutMs;
	}

	////////////////////////////////////////
	// Getters
	const unsigned char *getData() const { return _pData; }

	size_t getLength() const { return _length; }

	////////////////////////////////////////
	// Disable copy constructor and copy assignment operator
	// QueueData(const QueueData &) = delete;
	// QueueData &operator=(const QueueData &) = delete;

	// // Enable move constructor and move assignment operator
	// QueueData(QueueData &&other) noexcept
	// 	: _pData(other._pData), _length(other._length)
	// {
	// 	other._pData = nullptr;
	// 	other._length = 0;
	// }

	// QueueData &operator=(QueueData &&other) noexcept
	// {
	// 	if (this != &other)
	// 	{
	// 		if( _pData != nullptr )
	// 			delete[] _pData;
	// 		_pData = other._pData;
	// 		_length = other._length;
	// 		other._pData = nullptr;
	// 		other._length = 0;
	// 	}
	// 	return *this;
	// }
};

#endif // QUEUE_DATA_H