//////////////////////////////////////////////////////////////////////////////
// Byte stream class
// 
// Represents a stream (array) of bytes, as well as methods for manipulating
// the data on byte or bit level.
//////////////////////////////////////////////////////////////////////////////
#ifndef HEADER_BYTESTREAM
#define HEADER_BYTESTREAM

#include <vector>
#include <string>

using bitstream_index = long long;

class ByteStream
{
	private:
		// Data members
		std::vector<char> _data;
		unsigned short _nextBit;
		unsigned int _byteFrequency[256];
		double _byteProbability[256];
		bool _bytesChanged;		// Dirty flag

		// Private methods
		void BytesChanged();

	public:
		// Constructor / destructor
		ByteStream();
		ByteStream(const ByteStream& stream);
		~ByteStream();

		// Operator overloads
		char& operator[](unsigned int index);					// Indexing
		const char operator[](unsigned int index) const;		// Const version
		const ByteStream& operator=(const ByteStream& stream);	// Copy-assignment

		// Iterators
		decltype(_data.cbegin()) cbegin() const;
		decltype(_data.begin()) begin();
		decltype(_data.cend()) cend() const;
		decltype(_data.end()) end();

		// Analysis methods
		double byte_entropy() const;
		double bit_entropy() const;
		unsigned int byte_frequency(int byte) const;
		double byte_probability(int byte) const;
		double byte_information_content(int byte) const;

		// Bit manipulation methods
		void put(char datum, unsigned short bits = 8);
		char read(bitstream_index firstBit, unsigned short bits = 8) const;
		void clear();

		// Other public methods
		void bytes_changed(bool forceImmediateUpdate = true);
		bool load(const std::string& filename);
		bool save(const std::string& filename);
		unsigned int size() const;
};

#endif