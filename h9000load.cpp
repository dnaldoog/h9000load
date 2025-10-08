#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <zlib.h>

// ==============================================
// Base64 Decoding Implementation (Helper functions unchanged)
// ==============================================

// Helper function to find the index of a Base64 character
int base64_char_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

std::vector<unsigned char> base64Decode(const std::string& encoded_string) {
    std::vector<unsigned char> decoded_bytes;
    size_t i = 0;
    while (i < encoded_string.length()) {
        int val[4];
        int count = 0;
        for (int j = 0; j < 4 && i < encoded_string.length(); ++i) {
            char c = encoded_string[i];
            if (c == '=') {
                val[j] = 0;
                j++;
                break;
            }
            int index = base64_char_index(c);
            if (index != -1) {
                val[j] = index;
                j++;
                count++;
            }
        }

        if (count == 0) break;

        if (count >= 2) decoded_bytes.push_back((unsigned char)(val[0] << 2 | val[1] >> 4));
        if (count >= 3) decoded_bytes.push_back((unsigned char)(val[1] << 4 | val[2] >> 2));
        if (count == 4) decoded_bytes.push_back((unsigned char)(val[2] << 6 | val[3]));
    }
    return decoded_bytes;
}

// ==============================================
// Main function with file-to-file logic
// ==============================================

int main() {
    std::string filename;
    std::cout << "Enter the .9ks filename to decode: ";
    std::getline(std::cin, filename);

    // --- INPUT FILE HANDLING ---
    std::ifstream input_file(filename);
    if (!input_file.is_open()) {
        std::cerr << "Error: Could not open input file '" << filename << "'" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << input_file.rdbuf();
    std::string base64_gzipped_data = buffer.str();
    input_file.close();

    // Clean the string (remove potential newlines/spaces)
    base64_gzipped_data.erase(
        std::remove_if(base64_gzipped_data.begin(), base64_gzipped_data.end(), ::isspace),
        base64_gzipped_data.end());

    if (base64_gzipped_data.empty()) {
        std::cerr << "Error: File content is empty or only contains whitespace." << std::endl;
        return 1;
    }

    // Step 1: Base64 decode
    std::vector<unsigned char> decoded_data = base64Decode(base64_gzipped_data);

    if (decoded_data.empty()) {
        std::cerr << "Error: Base64 decoding produced empty data. Check file content validity." << std::endl;
        return 1;
    }

    // Step 2: Gzip decompress (Zlib logic remains the same)
    std::vector<unsigned char> decompressed_data;
    const size_t CHUNK_SIZE = 16384;
    z_stream strm = {};
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if (inflateInit2(&strm, 32 + MAX_WBITS) != Z_OK) {
        std::cerr << "Failed to initialize inflate." << std::endl;
        return 1;
    }

    strm.avail_in = decoded_data.size();
    strm.next_in = decoded_data.data();

    int ret;
    do {
        std::vector<unsigned char> out_buffer(CHUNK_SIZE);
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = out_buffer.data();

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret < 0 && ret != Z_BUF_ERROR) {
            std::cerr << "Gzip decompression error: " << strm.msg << std::endl;
            inflateEnd(&strm);
            return 1;
        }
        decompressed_data.insert(decompressed_data.end(), out_buffer.begin(), out_buffer.begin() + (CHUNK_SIZE - strm.avail_out));
    } while (strm.avail_out == 0);

    inflateEnd(&strm);

    if (decompressed_data.empty()) {
        std::cerr << "Error: Decompression produced empty data." << std::endl;
        return 1;
    }

    // --- OUTPUT FILE NAME CREATION ---
    std::string output_filename = filename;
    // Find the last dot in the filename (the start of the extension)
    size_t last_dot = output_filename.find_last_of('.');

    if (last_dot != std::string::npos) {
        // Replace the existing extension with ".json"
        output_filename.replace(last_dot, std::string::npos, ".json");
    }
    else {
        // If no extension, just append ".json"
        output_filename += ".json";
    }

    // --- OUTPUT FILE WRITING ---
    std::ofstream output_file(output_filename);
    if (!output_file.is_open()) {
        std::cerr << "Error: Could not open output file '" << output_filename << "'" << std::endl;
        return 1;
    }

    // Write the decompressed data (as a string) to the new file
    output_file.write(reinterpret_cast<const char*>(decompressed_data.data()), decompressed_data.size());
    output_file.close();

    std::cout << "Successfully decoded '" << filename << "' to '" << output_filename << "'" << std::endl;

    return 0;
}