//////////////////////////////////////////////////////////////////////////////
// Simple/naïve compression algorithm
//////////////////////////////////////////////////////////////////////////////
#ifndef HEADER_COMPRESSION_SIMPLE
#define HEADER_COMPRESSION_SIMPLE

#include <map>

#include "ByteStreamEncoder.h"

class SimpleCompression : public ByteStreamEncoder
{
	using codeword_pair = std::pair<int, unsigned short>;
	using encoding_map = std::map<char, std::pair<int, unsigned short>>;
	using decoding_map = std::map<codeword_pair, unsigned int>;

	private:
		// Data members
		double _targetFraction;

		// Private methods
		bool ReadMapFromKeyStream(encoding_map& map, int& bits_short, int& bits_long);
		void GetDecodingMap(const encoding_map& emap, decoding_map& dmap);

	public:
		// Constructor / destructor
		SimpleCompression(const ByteStream& inStream, ByteStream& outStream, ByteStream& keyStream);
		~SimpleCompression();

		// Public ByteStreamEncoder interface
		bool Encode() override;
		bool Decode() override;
		bool UsesKey() const override;
		std::string Name() const override;
		bool GenerateKey() override;

		// Other public methods
		void SetTargetFraction(double targetFraction);
};

#endif