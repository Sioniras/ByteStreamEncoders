//////////////////////////////////////////////////////////////////////////////
// Main file
//////////////////////////////////////////////////////////////////////////////
#include <iostream>

#include "include/ByteStream.h"
#include "include/ByteStreamEncoder.h"
#include "include/SimpleCompression.h"

int main()
{
	bool generate_key = true;

	// Filenames used for testing
	const std::string in_testfile = "../assets/molspin_source.txt";
	const std::string out_encoded_testfile = "../assets/molspin_source.encoded";
	const std::string out_decoded_testfile = "../assets/molspin_source.decoded";
	const std::string keyfile = "../assets/encoding_map.key";

	// Setup byte stream for a file
	ByteStream inputStream;
	if (inputStream.load(in_testfile))
		std::cout << "Loaded file \"" << in_testfile << "\"." << std::endl;
	else
		std::cout << "Failed to load file \"" << in_testfile << "\"!" << std::endl;

	// Update statistics
	inputStream.bytes_changed();

	// We create a separate stream for the output file
	ByteStream outputStream;

	// And a key stream
	ByteStream keyStream;

	// Setup compression algorithm
	std::unique_ptr<ByteStreamEncoder> encoder = std::make_unique<SimpleCompression>(inputStream, outputStream, keyStream);

	// Generate a key
	if (generate_key)
	{
		if (inputStream.size() > 0 && encoder->GenerateKey())
		{
			if (keyStream.save(keyfile))
				std::cout << "Generated encoding map and saved it to \"" << keyfile << "\"." << std::endl;
			else
				std::cout << "Failed to save generated encoding map to file \"" << keyfile << "\"!" << std::endl;
		}
		else
		{
			std::cout << "Failed to generate key!" << std::endl;
		}
	}
	else
	{
		// Or load a key instead
		if (keyStream.load(keyfile))
			std::cout << "Loaded key \"" << keyfile << "\"." << std::endl;
		else
			std::cout << "Failed to load key \"" << keyfile << "\"!" << std::endl;
	}

	if (inputStream.size() > 0)
	{
		// Input file statistics
		std::cout << "\n--- Input file statistics:\n";
		std::cout << "  - File size: " << inputStream.size() << " bytes\n";
		std::cout << "  - File entropy (bytes): " << inputStream.byte_entropy() << " bits\n";
		std::cout << "  - File entropy (bits): " << inputStream.bit_entropy() << " bits\n" << std::endl;

		// Perform the byte stream manipulations
		if (encoder->Encode())
		{
			std::cout << "Successfully encoded file!\n" << std::endl;

			double compression_ratio = static_cast<double>(outputStream.size()) / static_cast<double>(inputStream.size());

			// Output file statistics
			std::cout << "--- Encoded output file statistics:\n";
			std::cout << "  - Algorithm: " << encoder->Name() << "\n";
			std::cout << "  - File size: " << outputStream.size() << " bytes\n";
			std::cout << "  - Compression ratio: " << compression_ratio << "\n";
			std::cout << "  - File size reduction: " << (100.0 - compression_ratio * 100.0) << "%\n";
			std::cout << "  - File entropy (bytes): " << outputStream.byte_entropy() << " bits\n";
			std::cout << "  - File entropy (bits): " << outputStream.bit_entropy() << " bits\n" << std::endl;

			// Write the output stream to a file
			outputStream.save(out_encoded_testfile);
		}
		else
		{
			std::cout << "Failed to encode file!" << std::endl;
		}

		// Also decode the file again
		std::cout << " -------- Decoding file --------" << std::endl;
		inputStream = outputStream;
		if (encoder->Decode())
		{
			std::cout << "Successfully decoded file!\n" << std::endl;

			// Output file statistics
			std::cout << "--- Decoded output file statistics:\n";
			std::cout << "  - Algorithm: " << encoder->Name() << "\n";
			std::cout << "  - File size: " << outputStream.size() << " bytes\n";
			std::cout << "  - File entropy (bytes): " << outputStream.byte_entropy() << " bits\n";
			std::cout << "  - File entropy (bits): " << outputStream.bit_entropy() << " bits\n" << std::endl;

			// Write the output stream to a file
			outputStream.save(out_decoded_testfile);
		}
		else
		{
			std::cout << "Failed to decode file!" << std::endl;
		}
	}
	else
	{
		std::cout << "No bytes read from file.";
	}

	// Keep console open to display results
	std::cin.get();

	return 0;
}