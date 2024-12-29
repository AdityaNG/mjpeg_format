# MJPEG (Motion JPEG) Format Specification

by AdityaNG

## 1. Overview
MJPEG (Motion JPEG) is a video format where each frame is independently compressed as a JPEG image. This format sacrifices compression efficiency for simplicity and editing capability.

## 2. File Structure

### 2.1 Basic Layout
```
[JPEG Frame 1][JPEG Frame 2][JPEG Frame 3]...[JPEG Frame N]
```
- Each frame is a complete, valid JPEG file
- Frames are concatenated directly with no additional headers or separators
- No global file header is required
- No index table is needed

### 2.2 Frame Structure
Each frame must be a valid JPEG image following the JPEG specification with:
- Start of Image (SOI) marker: `FF D8`
- End of Image (EOI) marker: `FF D9`

### 2.3 Required JPEG Segments (in order)
1. SOI (Start of Image)
   - Marker: `FF D8`
   - Length: 2 bytes

2. APP0 (JFIF header)
   - Marker: `FF E0`
   - Length: Variable (typically 16 bytes)
   - Content must include:
     - "JFIF" identifier
     - Version number
     - Resolution information

3. DQT (Define Quantization Table)
   - Marker: `FF DB`
   - Length: Variable

4. SOF0 (Start of Frame, Baseline DCT)
   - Marker: `FF C0`
   - Contains image dimensions and components

5. DHT (Define Huffman Table)
   - Marker: `FF C4`
   - Length: Variable

6. SOS (Start of Scan)
   - Marker: `FF DA`
   - Followed by compressed image data

7. EOI (End of Image)
   - Marker: `FF D9`

## 3. Implementation Requirements

### 3.1 Frame Requirements
- Resolution must be consistent across all frames
- Color space must be consistent (recommended: YUV420)
- Each frame must be independently decodable
- Maximum resolution: Implementation defined
- Minimum resolution: 1x1 pixels

### 3.2 Timing
- No embedded timing information
- Frame rate is determined by the player/container
- Default assumption: 25 fps unless specified otherwise

### 3.3 Color Space
- YUV420 (JFIF standard) recommended
- RGB color space allowed but not recommended
- Color space must remain consistent throughout the video

## 4. Writing Implementation

### 4.1 Basic Algorithm
```c
void write_mjpeg(FILE* output, const jpeg_frame* frames[], int frame_count) {
    for (int i = 0; i < frame_count; i++) {
        // Write complete JPEG frame
        fwrite(frames[i].data, 1, frames[i].size, output);
    }
}
```

### 4.2 Frame Validation Steps
1. Verify SOI marker at start
2. Verify EOI marker at end
3. Check frame dimensions match video dimensions
4. Ensure all required JPEG segments are present
5. Verify markers are properly formatted

### 4.3 Error Handling
- Implementation must handle marker stuffing (0xFF followed by 0x00)
- Invalid frames should trigger error
- Missing EOI marker should trigger error
- Corrupted JPEG data should trigger error

## 5. Reading Implementation

### 5.1 Basic Algorithm
```c
int read_mjpeg_frame(FILE* input, jpeg_frame* frame) {
    uint8_t marker[2];
    
    // Find SOI marker
    do {
        if (fread(marker, 1, 2, input) != 2) return EOF;
    } while (marker[0] != 0xFF || marker[1] != 0xD8);
    
    // Read until EOI marker
    frame->data[0] = marker[0];
    frame->data[1] = marker[1];
    frame->size = 2;
    
    while (1) {
        if (fread(marker, 1, 2, input) != 2) return ERROR;
        frame->data[frame->size++] = marker[0];
        frame->data[frame->size++] = marker[1];
        
        if (marker[0] == 0xFF && marker[1] == 0xD9) {
            return SUCCESS;
        }
    }
}
```

### 5.2 Frame Detection
- Look for SOI marker (FF D8)
- Read until EOI marker (FF D9)
- Handle marker stuffing (FF 00)
- Verify JPEG structure

## 6. Common Pitfalls

### 6.1 Implementation Warnings
- Don't assume frames are equal size
- Don't assume fixed offset between frames
- Don't rely on marker stuffing for frame separation
- Don't assume consistent JPEG structure between frames
- Handle buffer overflows when reading frames

### 6.2 Common Errors
- Missing EOI markers
- Incorrect handling of FF 00 sequences
- Buffer overflow in frame reading
- Incorrect frame size calculation
- Assumption of fixed frame size

## 7. Testing

### 7.1 Minimum Test Cases
1. Single frame video
2. Multiple identical frames
3. Frames with different sizes
4. Maximum supported resolution
5. Minimum supported resolution
6. Different color spaces
7. Missing EOI markers
8. Corrupted frames

### 7.2 Validation Steps
1. Frame count matches expected
2. Frame dimensions consistent
3. Each frame independently decodable
4. Color space consistent
5. No data corruption between frames

## 8. Performance Considerations

### 8.1 Writing Optimization
- Buffer writes to reduce I/O operations
- Validate frames before writing
- Pre-allocate buffers when possible

### 8.2 Reading Optimization
- Buffer reads in chunks
- Use memory mapping for large files
- Implement frame skipping for seeking

# Code Walkthrough

1. **JPEGValidator class**
   - Checks for valid JPEG markers (SOI at start, EOI at end)
   - Ensures basic file integrity

2. **MJPEGWriter class**
   - Manages the output MJPEG file
   - Validates frame dimensions for consistency
   - Handles frame writing and error checking

3. **File Processing**
   - Uses C++17's filesystem library for directory operations
   - Sorts files alphabetically
   - Supports both .jpg and .jpeg extensions

4. **Error Handling**
   - Validates input directory exists
   - Checks for empty directories
   - Reports individual frame processing failures
   - Provides detailed error messages

To compile and use the program:

```bash
# Generate your frame sequence
ffmpeg -i sample_video.mjpeg sample_images/frame_%04d.jpg

# Compile (with C++17 or later)
g++ -std=c++17 mjpeg_creator.cpp -o mjpeg_creator

# Run
./mjpeg_creator sample_images/ output.mjpeg
```

The program will:
1. Scan the input directory for JPEG files
2. Sort them alphabetically
3. Validate each JPEG file
4. Ensure all frames have the same dimensions
5. Concatenate them into a single MJPEG file
6. Print a summary of the operation

# Analysis

Play the two videos
```
ffplay sample_video.mjpeg
ffplay output.mjpeg
```

You can view the video's analysis at [analysis_sample_video.txt](analysis_sample_video.txt) and [analysis_output.txt](analysis_output.txt).
```
./video_analysis.sh sample_video.mjpeg 
./video_analysis.sh output.mjpeg 
```
