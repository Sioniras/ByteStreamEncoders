//////////////////////////////////////////////////////////////////////////////
// Byte stream encoder class
// 
// Base class for byte stream encoders. New algorithms for compression,
// encryption, etc. should be encapsulated in classes inheriting this class.
//////////////////////////////////////////////////////////////////////////////
#ifndef HEADER_BYTESTREAM_ENCODER
#define HEADER_BYTESTREAM_ENCODER

#include "ByteStream.h"

class ByteStreamEncoder
{
	protected:
		// Data members
		const ByteStream& _inStream;
		ByteStream& _outStream;
		ByteStream& _keyStream;

	public:
		// Constructor / destructor
		ByteStreamEncoder(const ByteStream& inStream, ByteStream& outStream, ByteStream& keyStream);
		virtual ~ByteStreamEncoder();

		// Public interface
		virtual bool Encode() = 0;
		virtual bool Decode() = 0;
		virtual bool UsesKey() const = 0;
		virtual std::string Name() const = 0;
		virtual bool GenerateKey();

		// Other public methods
		void SetKeyStream(ByteStream& keyStream);
};

#endif