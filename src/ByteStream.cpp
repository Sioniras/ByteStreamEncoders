//////////////////////////////////////////////////////////////////////////////
// Byte stream implementation
//////////////////////////////////////////////////////////////////////////////
#include <cassert>
#include <fstream>

#include "..\include\ByteStream.h"

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------
// Constructor
ByteStream::ByteStream() : _data(), _nextBit(0), _byteFrequency{0}, _byteProbability{0}, _bytesChanged(true)
{
}

// Copy-constructor
ByteStream::ByteStream(const ByteStream& stream) : _data(stream._data), _nextBit(stream._nextBit), _byteFrequency{0}, _byteProbability{0}, _bytesChanged(true)
{
}

// Destructor
ByteStream::~ByteStream()
{
}

// ---------------------------------------------------------------------------
// Operators
// ---------------------------------------------------------------------------
// Indexing operator
// NOTE: Provides direct access to internal resource managed by the stream.
//       This is intended, but such access is not setting the dirty flag!
char& ByteStream::operator[](unsigned int index)
{
	return _data[index];
}

// Const version
const char ByteStream::operator[](unsigned int index) const
{
	return _data[index];
}

// Copy-assignment
const ByteStream& ByteStream::operator=(const ByteStream& stream)
{
	_data = stream._data;
	_nextBit = stream._nextBit;
	_bytesChanged = true;
	return *this;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
// Update internal data members when the bytes are changed
void ByteStream::BytesChanged()
{
	// Reset byte frequencies
	memset(_byteFrequency, 0, sizeof(unsigned int) * 256);

	// Compute byte frequencies
	for (auto i = _data.cbegin(); i != _data.cend(); i++)
		++_byteFrequency[static_cast<unsigned int>(*i) & 0xFF];

	// Get probabilities
	for (auto i = 0; i < 256; i++)
		_byteProbability[i] = static_cast<double>(_byteFrequency[i]) / static_cast<double>(_data.size());

	// Reset dirty flag
	_bytesChanged = false;
}

// ---------------------------------------------------------------------------
// Iterators
// ---------------------------------------------------------------------------
// NOTE: Provides direct access to internal resource managed by the stream.
//       This is intended, but such access is not setting the dirty flag!
decltype(ByteStream::_data.cbegin()) ByteStream::cbegin() const
{
	return _data.cbegin();
}

decltype(ByteStream::_data.begin()) ByteStream::begin()
{
	return _data.begin();
}

decltype(ByteStream::_data.cend()) ByteStream::cend() const
{
	return _data.cend();
}

decltype(ByteStream::_data.end()) ByteStream::end()
{
	return _data.end();
}

// ---------------------------------------------------------------------------
// Analysis methods
// ---------------------------------------------------------------------------
// Calculates the Shannon entropy of the byte stream
double ByteStream::byte_entropy() const
{
	// Make sure the statistics are prepared
	assert(!_bytesChanged);

	// Get the entropy
	double entropy = 0;
	for (auto i = 0; i < 256; i++)
		if (_byteFrequency[i] > 0)
			entropy -= _byteProbability[i] * std::log2(_byteProbability[i]);

	return entropy;
}

// Calculates the Shannon entropy of the bits in the stream
double ByteStream::bit_entropy() const
{
	// Make sure the statistics are prepared
	assert(!_bytesChanged);

	// Get the entropy
	unsigned int ones = 0;
	for (auto i = _data.cbegin(); i != _data.cend(); i++)
	{
		ones += (*i & 0b00000001) ? 1 : 0;
		ones += (*i & 0b00000010) ? 1 : 0;
		ones += (*i & 0b00000100) ? 1 : 0;
		ones += (*i & 0b00001000) ? 1 : 0;
		ones += (*i & 0b00010000) ? 1 : 0;
		ones += (*i & 0b00100000) ? 1 : 0;
		ones += (*i & 0b01000000) ? 1 : 0;
		ones += (*i & 0b10000000) ? 1 : 0;
	}

	double one_probability = (static_cast<double>(ones) / static_cast<double>(_data.size())) / 8.0;
	double zero_probability = 1.0 - one_probability;

	return -one_probability * std::log2(one_probability) - zero_probability * std::log2(zero_probability);
}

// Returns the number of times a specific byte value is present in the byte stream
unsigned int ByteStream::byte_frequency(int byte) const
{
	// Make sure the byte index is valid
	assert(byte >= 0 && byte < 256);

	// Make sure the statistics are prepared
	assert(!_bytesChanged);

	return _byteFrequency[byte];
}

// Returns the probability of getting a specific byte value when picking a random byte from the stream
double ByteStream::byte_probability(int byte) const
{
	// Make sure the byte index is valid
	assert(byte >= 0 && byte < 256);

	// Make sure the statistics are prepared
	assert(!_bytesChanged);

	return _byteProbability[byte];
}

// Returns the quantified information contributed by the presence of a specific byte in the stream
double ByteStream::byte_information_content(int byte) const
{
	// Make sure the byte index is valid
	assert(byte >= 0 && byte < 256);

	// Make sure the statistics are prepared
	assert(!_bytesChanged);

	return -std::log2(_byteProbability[byte]);
}

// ---------------------------------------------------------------------------
// Population methods
// ---------------------------------------------------------------------------
// Adds a number of bits to the stream
void ByteStream::put(char datum, unsigned short bits)
{
	// Only 8 bits can fit into the datum
	assert(bits <= 8 && bits > 0);

	// Get only the requested bits from the datum
	char new_bits = datum & (0xFF >> (8-bits));

	// Get the bitwise shift that would correctly align the bits
	const int shift = 8 - (bits + _nextBit);
	if (shift < 0)
		new_bits = (new_bits >> (-shift)) & (0xFF >> (-shift));
	else
		new_bits = (new_bits << shift) & 0xFF;

	// Check whether an "unfinished" byte is already present in the array
	if (_nextBit > 0 && _data.size() > 0)
	{
		// Add the bits
		_data[_data.size() - 1] |= new_bits;
		_nextBit += bits;

		// If there was not enough room in the byte, add the remaining bits to a new byte
		if (_nextBit > 8)
		{
			auto bit_overflow = _nextBit - 8;	// Equal to -shift
			_nextBit = 0;
			char remaining_bits = datum & (0xFF >> (8 - bit_overflow));
			put(remaining_bits, bit_overflow);
		}
		else if (_nextBit == 8)
		{
			_nextBit = 0;
		}
	}
	else
	{
		// Otherwise add a new byte
		_data.push_back(new_bits);

		// Check whether all bits are used
		if(bits < 8)
			_nextBit = bits;
	}
}

// Read upto 8 bits from the stream, from a given bit index
char ByteStream::read(bitstream_index firstBit, unsigned short bits) const
{
	// If no bytes were requested
	if (bits < 1)
		return (char)0;

	// Only 8 bits can be fit into the return type
	assert(bits <= 8);

	// Get the index of the byte with the first bit to return,
	// and the index of the first bit within that byte to return
	auto byte_index = firstBit / 8;
	auto bit_index = firstBit % 8;

	// Get the requested bits from the byte with the first bit
	assert(_data.size() > byte_index);
	char result = _data[byte_index] << bit_index;

	int shift = 8 - (bits + bit_index);
	if (shift >= 0)
	{
		result &= (0xFF << shift) & 0xFF;
	}
	else
	{
		assert(_data.size() > byte_index + 1);
		char bitsFromNextByte = _data[byte_index + 1] & (0xFF << (8+shift));
		result |= (bitsFromNextByte >> (8 - bit_index)) & (0xFF >> (8 - bit_index));
	}

	result = result >> (8 - bits);
	result &= 0xFF >> (8 - bits);

	return result;
}

// Removes all data from the byte stream
void ByteStream::clear()
{
	_data.clear();
	_nextBit = 0;
	_bytesChanged = true;
}

// ---------------------------------------------------------------------------
// Other public methods
// ---------------------------------------------------------------------------
// Sets the dirty flag and allows forcing of update
void ByteStream::bytes_changed(bool forceImmediateUpdate)
{
	_bytesChanged = true;

	if (forceImmediateUpdate)
		BytesChanged();
}

// Loads a file into the buffer, discarding any prior data in the buffer
bool ByteStream::load(const std::string& filename)
{
	std::ifstream filehandle(filename.c_str(), std::ios::in | std::ios::binary);

	if (!filehandle.good() || !filehandle.is_open())
		return false;

	// Recalculate byte statistics
	_bytesChanged = true;

	// Delete previous data if any
	_data.clear();

	// All bits are used for each byte
	_nextBit = 0;

	// Get file size and allocate memory
	filehandle.seekg(0, filehandle.end);
	unsigned int filesize = filehandle.tellg();
	filehandle.seekg(0, filehandle.beg);
	_data.reserve(filesize);

	// Read bytes
	char c;
	while (filehandle.get(c))
		_data.push_back(c);

	return true;
}

// Saves the content of the buffer to a file
bool ByteStream::save(const std::string& filename)
{
	std::ofstream filehandle(filename.c_str(), std::ios::out | std::ios::binary);

	if (!filehandle.good() || !filehandle.is_open())
		return false;

	for (auto i = _data.cbegin(); i != _data.cend(); i++)
		filehandle.put(*i);

	return true;
}

// Returns the number of bytes in the stream
unsigned int ByteStream::size() const
{
	return _data.size();
}
// ---------------------------------------------------------------------------
