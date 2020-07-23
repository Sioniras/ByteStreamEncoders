//////////////////////////////////////////////////////////////////////////////
// Byte stream encoder implementation
//////////////////////////////////////////////////////////////////////////////
#include <cassert>

#include "..\include\ByteStreamEncoder.h"

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------
// Constructor
ByteStreamEncoder::ByteStreamEncoder(const ByteStream& inStream, ByteStream& outStream, ByteStream& keyStream)
	:	_inStream(inStream),
		_outStream(outStream),
		_keyStream(keyStream)
{
}

// Destructor
ByteStreamEncoder::~ByteStreamEncoder()
{
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------
// Not all encoders have to use keys
bool ByteStreamEncoder::GenerateKey()
{
	assert(UsesKey());
	return false;
}

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
// Keys may be read/written on decoding/encoding if keys are used
void ByteStreamEncoder::SetKeyStream(ByteStream& keyStream)
{
	_keyStream = keyStream;
}
// ---------------------------------------------------------------------------
