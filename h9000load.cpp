#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <zlib.h>

// ==============================================
// 1. Base64 Encoding/Decoding Functions (Re-used and new)
// ==============================================

// Helper function to find the index of a Base64 character (for decoding)
int base64_char_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

// Base64 Alphabet for Encoding
const char* BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// New: Base64 Encoding Function
std::string base64Encode(const std::vector<unsigned char>& data) {
    std::string encoded_string;
    size_t i = 0;
    while (i < data.size()) {
        int val = 0;
        int bits = 0;
        int count = 0;

        // Take up to 3 bytes (24 bits)
        for (int j = 0; j < 3; ++j) {
            if (i < data.size()) {
                val = (val << 8) + data[i];
                bits += 8;
                i++;
                count++;
            }
            else {
                val <<= 8; // Pad with zero
            }
        }

        // Convert 24 bits into 4 Base64 characters (6 bits each)
        for (int j = 0; j < 4; ++j) {
            if (bits > 0) {
                encoded_string += BASE64_CHARS[(val >> (bits - 6)) & 0x3f];
                bits -= 6;
            }
            else if (count > 0) {
                encoded_string += '='; // Add padding if needed
            }
        }
    }
    return encoded_string;
}

// Re-used: Base64 Decoding Function
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
// 2. Core Decompress Function (Based on previous logic)
// ==============================================

int decompress_file(const std::string& input_path) {
    // 1. Prepare filenames
    std::string output_path = input_path;
    size_t last_dot = output_path.find_last_of('.');
    if (last_dot != std::string::npos) {
        output_path.replace(last_dot, std::string::npos, ".json");
    }
    else {
        output_path += ".json";
    }

    // 2. Read Base64 input file
    std::ifstream input_file(input_path);
    if (!input_file.is_open()) {
        std::cerr << "Error: Could not open input file '" << input_path << "'" << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << input_file.rdbuf();
    std::string base64_gzipped_data = buffer.str();
    input_file.close();

    base64_gzipped_data.erase(
        std::remove_if(base64_gzipped_data.begin(), base64_gzipped_data.end(), ::isspace),
        base64_gzipped_data.end());

    if (base64_gzipped_data.empty()) {
        std::cerr << "Error: Input file is empty or only contains whitespace." << std::endl;
        return 1;
    }

    // 3. Base64 Decode
    std::vector<unsigned char> decoded_data = base64Decode(base64_gzipped_data);
    if (decoded_data.empty()) {
        std::cerr << "Error: Base64 decoding failed or resulted in empty data." << std::endl;
        return 1;
    }

    // 4. Gzip Decompress
    std::vector<unsigned char> decompressed_data;
    const size_t CHUNK_SIZE = 16384;
    z_stream strm = {};
    strm.zalloc = Z_NULL; strm.zfree = Z_NULL; strm.opaque = Z_NULL;

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
    } while (strm.avail_out == 0 && ret != Z_STREAM_END); // Check for Z_STREAM_END

    inflateEnd(&strm);

    if (decompressed_data.empty()) {
        std::cerr << "Error: Decompression produced empty data." << std::endl;
        return 1;
    }

    // 5. Write JSON output file
    std::ofstream output_file(output_path);
    if (!output_file.is_open()) {
        std::cerr << "Error: Could not open output file '" << output_path << "'" << std::endl;
        return 1;
    }

    output_file.write(reinterpret_cast<const char*>(decompressed_data.data()), decompressed_data.size());
    output_file.close();

    std::cout << "SUCCESS: Decompressed '" << input_path << "' to '" << output_path << "'" << std::endl;
    return 0;
}

// ==============================================
// 3. New Core Compress Function
// ==============================================

int compress_file(const std::string& input_path) {
    // 1. Prepare input and output names
    std::string output_path = input_path;
    size_t last_dot = output_path.find_last_of('.');
    if (last_dot != std::string::npos) {
        // Remove the old extension (e.g., .json) and add a suffix and the new extension (.9ks)
        output_path = output_path.substr(0, last_dot) + "_out.9ks";
    }
    else {
        // If no extension, just add the suffix and the new extension
        output_path += "_out.9ks";
    }

    // 2. Read JSON input file
    std::ifstream input_file(input_path, std::ios::binary);
    if (!input_file.is_open()) {
        std::cerr << "Error: Could not open input file '" << input_path << "'" << std::endl;
        return 1;
    }
    std::vector<unsigned char> input_data(
        (std::istreambuf_iterator<char>(input_file)),
        std::istreambuf_iterator<char>());
    input_file.close();

    if (input_data.empty()) {
        std::cerr << "Error: Input JSON file is empty." << std::endl;
        return 1;
    }

    // 3. Gzip Compress
    std::vector<unsigned char> compressed_data;
    const size_t CHUNK_SIZE = 16384;
    z_stream strm = {};
    strm.zalloc = Z_NULL; strm.zfree = Z_NULL; strm.opaque = Z_NULL;

    // Initialize deflate for Gzip format (16 + windowBits for Gzip)
    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        std::cerr << "Failed to initialize deflate." << std::endl;
        return 1;
    }

    strm.avail_in = input_data.size();
    strm.next_in = input_data.data();

    int ret;
    do {
        std::vector<unsigned char> out_buffer(CHUNK_SIZE);
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = out_buffer.data();

        ret = deflate(&strm, Z_FINISH); // Z_FINISH tells zlib to complete the compression
        if (ret < 0) {
            std::cerr << "Gzip compression error: " << strm.msg << std::endl;
            deflateEnd(&strm);
            return 1;
        }
        compressed_data.insert(compressed_data.end(), out_buffer.begin(), out_buffer.begin() + (CHUNK_SIZE - strm.avail_out));
    } while (ret != Z_STREAM_END);

    deflateEnd(&strm);

    if (compressed_data.empty()) {
        std::cerr << "Error: Compression produced empty data." << std::endl;
        return 1;
    }

    // 4. Base64 Encode
    std::string base64_output = base64Encode(compressed_data);

    // 5. Write .9ks output file
    std::ofstream output_file(output_path);
    if (!output_file.is_open()) {
        std::cerr << "Error: Could not open output file '" << output_path << "'" << std::endl;
        return 1;
    }

    output_file << base64_output;
    output_file.close();

    std::cout << "SUCCESS: Compressed '" << input_path << "' to '" << output_path << "'" << std::endl;
    return 0;
}

// ==============================================
// 4. Main Argument Parsing
// ==============================================

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: h9000load -[d|c] <file_path>" << std::endl;
        std::cerr << "  -d: Decompress <file.9ks> to <file.json>" << std::endl;
        std::cerr << "  -c: Compress <file.json> to <file_out.9ks>" << std::endl;
        return 1;
    }


    std::string option = argv[1];
    std::string file_path = argv[2];

    if (option == "-d") {
        return decompress_file(file_path);
    }
    else if (option == "-c") {
        return compress_file(file_path);
    }
    else {
        std::cerr << "Error: Invalid option. Use -d (decompress) or -c (compress)." << std::endl;
        return 1;
    }
}