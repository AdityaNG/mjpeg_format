#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace fs = std::filesystem;

class JPEGValidator {
public:
    static bool isValidJPEG(const std::vector<uint8_t>& data) {
        if (data.size() < 4) return false;
        
        // Check SOI marker
        if (data[0] != 0xFF || data[1] != 0xD8) return false;
        
        // Check EOI marker at end
        if (data[data.size()-2] != 0xFF || data[data.size()-1] != 0xD9) return false;
        
        return true;
    }
};

class MJPEGWriter {
private:
    std::ofstream output;
    bool first_frame;
    uint32_t width;
    uint32_t height;

    bool validateDimensions(const std::vector<uint8_t>& jpeg_data) {
        // Find SOF0 marker (0xFF 0xC0)
        for (size_t i = 0; i < jpeg_data.size() - 8; i++) {
            if (jpeg_data[i] == 0xFF && jpeg_data[i+1] == 0xC0) {
                uint32_t frame_height = (jpeg_data[i+5] << 8) | jpeg_data[i+6];
                uint32_t frame_width = (jpeg_data[i+7] << 8) | jpeg_data[i+8];
                
                if (first_frame) {
                    width = frame_width;
                    height = frame_height;
                    first_frame = false;
                    return true;
                }
                
                return (frame_width == width && frame_height == height);
            }
        }
        return false;
    }

public:
    MJPEGWriter(const std::string& filename) : first_frame(true), width(0), height(0) {
        output.open(filename, std::ios::binary);
        if (!output.is_open()) {
            throw std::runtime_error("Could not open output file: " + filename);
        }
    }

    ~MJPEGWriter() {
        if (output.is_open()) {
            output.close();
        }
    }

    bool addFrame(const std::vector<uint8_t>& jpeg_data) {
        if (!JPEGValidator::isValidJPEG(jpeg_data)) {
            std::cerr << "Invalid JPEG data detected\n";
            return false;
        }

        if (!validateDimensions(jpeg_data)) {
            std::cerr << "Frame dimensions mismatch or SOF0 marker not found\n";
            return false;
        }

        output.write(reinterpret_cast<const char*>(jpeg_data.data()), jpeg_data.size());
        return true;
    }

    std::pair<uint32_t, uint32_t> getDimensions() const {
        return {width, height};
    }
};

std::vector<uint8_t> readFile(const fs::path& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath.string());
    }

    auto size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Could not read file: " + filepath.string());
    }

    return buffer;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_directory> <output_file.mjpeg>\n";
        return 1;
    }

    try {
        fs::path input_dir(argv[1]);
        if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
            throw std::runtime_error("Input directory does not exist or is not a directory");
        }

        // Collect and sort JPEG files
        std::vector<fs::path> jpeg_files;
        for (const auto& entry : fs::directory_iterator(input_dir)) {
            if (entry.path().extension() == ".jpg" || entry.path().extension() == ".jpeg") {
                jpeg_files.push_back(entry.path());
            }
        }
        std::sort(jpeg_files.begin(), jpeg_files.end());

        if (jpeg_files.empty()) {
            throw std::runtime_error("No JPEG files found in directory");
        }

        // Create MJPEG writer
        MJPEGWriter writer(argv[2]);

        // Process each file
        size_t processed = 0;
        for (const auto& jpeg_file : jpeg_files) {
            std::cout << "Processing: " << jpeg_file.filename() << std::endl;
            
            auto jpeg_data = readFile(jpeg_file);
            if (writer.addFrame(jpeg_data)) {
                processed++;
            } else {
                std::cerr << "Failed to add frame: " << jpeg_file.filename() << std::endl;
            }
        }

        auto [width, height] = writer.getDimensions();
        std::cout << "\nMJPEG creation complete:\n"
                  << "- Processed frames: " << processed << "/" << jpeg_files.size() << "\n"
                  << "- Resolution: " << width << "x" << height << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}