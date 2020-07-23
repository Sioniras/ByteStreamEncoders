//////////////////////////////////////////////////////////////////////////////
// Simple compression algorithm implementation
//////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <iomanip>
#include <cassert>
#include <list>

#include "..\include\SimpleCompression.h"

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------
// Constructor
SimpleCompression::SimpleCompression(const ByteStream& inStream, ByteStream& outStream, ByteStream& keyStream)
	:	ByteStreamEncoder(inStream, outStream, keyStream),
		_targetFraction(0.8)
{
}

// Destructor
SimpleCompression::~SimpleCompression()
{
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
// Retrieves data from the key stream
bool SimpleCompression::ReadMapFromKeyStream(encoding_map& map, int& bits_short, int& bits_long)
{
	// Make sure that the header bytes are available
	if (_keyStream.size() < 4)
		return false;

	// Get the header bytes
	int map_size = _keyStream[0];
	int short_words_count = _keyStream[1];
	bits_short = _keyStream[2];
	bits_long = _keyStream[3];

	// Make sure that enough data is available
	if (_keyStream.size() < 4 + static_cast<unsigned int>(map_size))
		return false;

	// Get a "pointer" to the next bit to read from the stream
	bitstream_index bit_ptr = 8 * 4;

	// Get all of the short codewords
	for (int i = 0; i < short_words_count; i++)
	{
		// Get the key byte
		char key = _keyStream.read(bit_ptr, 8);
		bit_ptr += 8;

		// Get the codeword
		unsigned int codeword = static_cast<unsigned int>(_keyStream.read(bit_ptr, bits_short)) & 0xFF;
		bit_ptr += bits_short;

		map[key] = codeword_pair(codeword, bits_short);
	}

	// Get all of the long codewords
	for (int i = short_words_count; i < map_size; i++)
	{
		// Get the key byte
		char key = _keyStream.read(bit_ptr, 8);
		bit_ptr += 8;

		// Get the codeword
		unsigned int codeword;
		if (bits_long > 8)
		{
			unsigned int codeword_part1 = static_cast<unsigned int>(_keyStream.read(bit_ptr, 8)) & 0xFF;
			unsigned int codeword_part2 = static_cast<unsigned int>(_keyStream.read(bit_ptr + 8, bits_long - 8)) & 0xFF;
			codeword = (codeword_part1 << (bits_long - 8)) | codeword_part2;
		}
		else
		{
			codeword = static_cast<unsigned int>(_keyStream.read(bit_ptr, bits_long)) & 0xFF;
		}
		bit_ptr += static_cast<bitstream_index>(bits_long);

		map[key] = codeword_pair(codeword, bits_long);
	}

	return true;
}

// Populate a decode map (i.e. invert key/value part of the data structure for the encoding map)
void SimpleCompression::GetDecodingMap(const encoding_map& encodingMap, decoding_map& decodingMap)
{
	// Make sure the decoding map is empty
	decodingMap.clear();

	// Fill the decoding map
	for (auto i = encodingMap.cbegin(); i != encodingMap.end(); i++)
		decodingMap[codeword_pair(i->second.first,i->second.second)] = i->first;
}

// ---------------------------------------------------------------------------
// Public ByteStreamEncoder interface
// ---------------------------------------------------------------------------
// Compression method
bool SimpleCompression::Encode()
{
	// Clear the output stream
	_outStream.clear();

	// Define parameters to be read from the key stream
	encoding_map character_map;
	int bits_short;
	int bits_long;

	// Read the key data
	if (!ReadMapFromKeyStream(character_map, bits_short, bits_long))
	{
		std::cout << "Failed to construct encoding map from key stream. Please make sure the key is valid!" << std::endl;
		return false;
	}

	// Iterate through each byte in the input file
	for (auto i = _inStream.cbegin(); i != _inStream.cend(); i++)
	{
		// Get the codeword for the current byte in the encoding map, and the number of bits in the codeword
		auto symbol = character_map[*i];
		unsigned short bits = symbol.second;

		// Put encoded bits into the output stream
		if (bits > 8)
		{
			// More than 8 bits (and assumed no more than 16 bits) requires 2 bytes to be stored
			char c1 = static_cast<char>((symbol.first >> 8) & (0xFF));
			char c2 = static_cast<char>(symbol.first & 0xFF);
			_outStream.put(c1, bits - 8);
			_outStream.put(c2, 8);
		}
		else
		{
			// Put the codeword (encoded byte/character) into the output stream
			_outStream.put(static_cast<char>(symbol.first & 0xFF), bits);
		}
	}

	// Update outstream statistics
	_outStream.bytes_changed();

	return true;
}

// Decompression method
bool SimpleCompression::Decode()
{
	// Clear the output stream
	_outStream.clear();

	// Define parameters to be read from the key stream
	encoding_map encoder;
	decoding_map decoder;
	int bits_short;
	int bits_long;

	// Read the key data
	if (!ReadMapFromKeyStream(encoder, bits_short, bits_long))
	{
		std::cout << "Failed to construct encoding map from key stream. Please make sure the key is valid!" << std::endl;
		return false;
	}

	// Get a decoding map (inverse encoding map)
	GetDecodingMap(encoder, decoder);
	
	int extended_bitset_key = static_cast<int>(std::pow(2, bits_short)) - 1;	// Bitsequence used to indicate that further bits are needed for the codeword
	int bitlength_difference = bits_long - bits_short;							// Number of additional bits used for the long codewords

	// Iterate through the bits in the bytestream
	bitstream_index bit_ptr = 0;	// "Pointer" (index) to the next bit in the bytestream
	while (bit_ptr + bits_short < static_cast<bitstream_index>(_inStream.size()) * 8)
	{
		// Get the next codeword
		unsigned int codeword = static_cast<unsigned int>(_inStream.read(bit_ptr, bits_short)) & 0xFF;
		bit_ptr += bits_short;
		unsigned int bitcount = bits_short;

		// Check whether the codeword is the key for extended codewords
		// Note: bits_long is zero if there are no extended codewords
		if (codeword == extended_bitset_key && bits_long != 0)
		{
			// End of file reached
			if (bit_ptr + bitlength_difference >= static_cast<bitstream_index>(_inStream.size()) * 8)
				break;

			// Read the remaining part of the codeword
			unsigned int codeword_extension = static_cast<unsigned int>(_inStream.read(bit_ptr, bitlength_difference)) & 0xFF;
			bit_ptr += bitlength_difference;

			// Get the complete codeword
			codeword = (codeword << bitlength_difference) | codeword_extension;
			bitcount = bits_long;
		}

		// Decode the codeword
		char decoded_byte = decoder[codeword_pair(codeword,bitcount)];

		// Add the decoded byte to the output stream
		_outStream.put(decoded_byte);
	}

	// Update outstream statistics
	_outStream.bytes_changed();

	return true;
}

// This method uses a key
bool SimpleCompression::UsesKey() const
{
	return true;
}

// Returns a string identifying the algorithm
std::string SimpleCompression::Name() const
{
	return "Simple compression algorithm";
}

// Fills the key stream with an encoding key
bool SimpleCompression::GenerateKey()
{
	std::cout << "\n -------- BEGIN GENERATING KEY --------" << std::endl;

	// Make a sorted list with the most used character as the first index
	std::list<char> ordering;
	for (int i = 0; i < 256; i++)
	{
		// Ignore bit combinations that are not present in the input file
		if (_inStream.byte_frequency(i) > 0)
		{
			bool has_inserted = false;

			// Just do a simple insertion sort
			for (auto j = ordering.cbegin(); j != ordering.cend(); j++)
			{
				if (_inStream.byte_frequency(i) > _inStream.byte_frequency(*j))
				{
					ordering.insert(j, (char)i);
					has_inserted = true;
					break;
				}
			}

			if (!has_inserted)
				ordering.push_back((char)i);
		}
	}

	int unique_bytes = static_cast<int>(ordering.size());	// Cast to signed int

	// ------ BEGIN ANALYSIS ------
	// Output info
	int bits_per_character = static_cast<int>(std::ceil(std::log2(unique_bytes)));
	std::cout << std::fixed << std::setprecision(2);
	std::cout << "\nNumber of unique bytes: " << unique_bytes << std::endl;
	std::cout << "Minimum (constant) number of bits per character: " << bits_per_character << std::endl;
	std::cout << "Total byte count with minimal (constant) bits per character: " << (static_cast<double>(bits_per_character) / 8.0 * _inStream.size());
	std::cout << " (" << (static_cast<double>(bits_per_character) / 8.0 * 100.0) << "% of original file)" << std::endl;
	std::cout << "Redundancy with minimal constant bits per character: " << (1.0 - static_cast<double>(unique_bytes) / static_cast<double>(1 << bits_per_character)) * 100.0;
	std::cout << "%\n" << std::endl;

	// Describe a percentage of the most used characters, this may take fewer bits
	int unique_upto_target_fraction = 0;
	double actual_fraction = 0;
	auto target_percentage_iterator = ordering.cbegin();
	for (; target_percentage_iterator != ordering.cend() && actual_fraction < _targetFraction; target_percentage_iterator++)
	{
		actual_fraction += _inStream.byte_probability(*target_percentage_iterator);
		unique_upto_target_fraction++;
	}
	int bits_per_target_character = static_cast<int>(std::ceil(std::log2(unique_upto_target_fraction + 1)));	// Add one here for extended codes

	if (bits_per_target_character < 2)
	{
		++bits_per_target_character;
		std::cout << "NOTE: Increased number of bits for target characters to minimal value " << bits_per_target_character << "." << std::endl;
	}

	// Extra characters above the percentage may be incluced to fill out all combinations
	int extra_characters = static_cast<int>(std::pow(2, bits_per_target_character)) - unique_upto_target_fraction - 1;
	for (int i = 0; target_percentage_iterator != ordering.cend() && i < extra_characters; i++)
	{
		actual_fraction += _inStream.byte_probability(*target_percentage_iterator);
		unique_upto_target_fraction++;
		target_percentage_iterator++;
	}

	// Update the number of characters not accounted for
	extra_characters = static_cast<int>(std::pow(2, bits_per_target_character)) - unique_upto_target_fraction - 1;

	// If there is only a single character missing, don't extend bitset
	if (unique_bytes - (unique_upto_target_fraction + extra_characters) == 1)
	{
		actual_fraction += _inStream.byte_probability(*target_percentage_iterator);
		unique_upto_target_fraction++;
		extra_characters = 0;
	}

	std::cout << "Target percentage: " << (_targetFraction * 100.0) << std::endl;
	std::cout << "Actual percentage: " << (actual_fraction * 100.0) << std::endl;
	std::cout << "Unique bytes within target: " << unique_upto_target_fraction << std::endl;
	std::cout << "Bits for target characters: " << bits_per_target_character << std::endl;

	// Describe amount of bits required for the last amount of characters
	int unique_remaining_characters = unique_bytes - unique_upto_target_fraction;
	int bits_per_remaining_characters = (unique_remaining_characters > 0) ? (bits_per_target_character + static_cast<int>(std::ceil(std::log2(unique_remaining_characters)))) : 0;
	std::cout << "Remaining characters: " << unique_remaining_characters << std::endl;
	std::cout << "Bits per remaining character: " << bits_per_remaining_characters << std::endl;
	std::cout << "Redundancy with this encoding: ";
	if (unique_remaining_characters > 0)
		std::cout << (1.0 - actual_fraction) * (1.0 - static_cast<double>(unique_remaining_characters) / static_cast<double>(1 << (bits_per_remaining_characters - bits_per_target_character))) * 100.0;
	else
		std::cout << (100.0 - static_cast<double>(unique_upto_target_fraction) / static_cast<double>(1 << bits_per_target_character) * 100.0);
	std::cout << "%" << std::endl;

	std::cout << "\n";
	// ------ END ANALYSIS ------

	// Clear key stream and set header bytes
	_keyStream.clear();
	_keyStream.put((char)unique_bytes);				// First byte is number of unique characters
	_keyStream.put((char)unique_upto_target_fraction);	// Second byte is number of characters with shortened bit length
	_keyStream.put(bits_per_target_character);			// Third byte is number of bits for shortened characters
	_keyStream.put(bits_per_remaining_characters);		// Fourth byte is number of bits for elongated characters

	// Prepare an iterator to run through the list of characters
	auto n = ordering.cbegin();

	// Write the shortened characters to the stream
	int i = 0;
	for (; i < unique_upto_target_fraction; i++)
	{
		_keyStream.put(*n);
		_keyStream.put(i, bits_per_target_character);
		++n;
	}

	// Write the elongated characters to the stream
	for (int j = 0; j + unique_upto_target_fraction < unique_bytes; j++)
	{
		_keyStream.put(*(n++));
		_keyStream.put(i, bits_per_target_character);
		_keyStream.put(j, bits_per_remaining_characters - bits_per_target_character);
	}

	std::cout << " -------- DONE GENERATING KEY --------" << std::endl;

	return true;
}

// ---------------------------------------------------------------------------
// Other public methods
// ---------------------------------------------------------------------------
void SimpleCompression::SetTargetFraction(double targetFraction)
{
	assert(targetFraction > 1e-10);
	assert(targetFraction < 1);
	_targetFraction = targetFraction;
}
