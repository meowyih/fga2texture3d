#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>

//
// BMP image functions
//
const int BYTES_PER_PIXEL = 3; // RGB without Alpha
const int FILE_HEADER_SIZE = 14;
const int INFO_HEADER_SIZE = 40;
const int MAX_SLICE_SIZE = 8; // how many slice in a single line, Godot 4.1 Texture3D only accept H:256 V:256 slice at most.

// help function to create BMP file header
unsigned char* createBitmapFileHeader(int height, int stride);

// help function to create BMP image header
unsigned char* createBitmapInfoHeader(int height, int width);


//
// help function to convert [0~0xFF] data to [-256.00, 256.00] data
// for debug purpose only
//
float remap(unsigned char data);

//
// parse a line which split the string token with delim (i.e.: ',' in FGA format)
//
template <typename Out>
void split(const std::string& s, char delim, Out result) {
	std::istringstream iss(s);
	std::string item;
	while (std::getline(iss, item, delim)) {
		*result++ = item;
	}
}

std::vector<std::string> split(const std::string& s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}


int main()
{
	std::string fga_filename("sample.fga");

	// open fga_filename file
	std::ifstream fin(fga_filename, std::ios::in);
	if (!fin.good())
	{
		std::cout << "err: failed to open fga file: " << fga_filename << std::endl;
		return -1;
	}

	// read fga_filename header
	float fwidth, fheight, fdepth, min[3], max[3];
	std::string line, seperator;
	std::vector<std::string> svec3;

	std::getline(fin, line);
	svec3 = split(line, ',');
	int width = (int)std::stof(svec3[0]);
	int height = (int)std::stof(svec3[1]);
	int depth = (int)std::stof(svec3[2]);

	std::getline(fin, line);
	svec3 = split(line, ',');
	min[0] = (int)std::stof(svec3[0]);
	min[1] = (int)std::stof(svec3[1]);
	min[2] = (int)std::stof(svec3[2]);

	std::getline(fin, line);
	svec3 = split(line, ',');
	max[0] = (int)std::stof(svec3[0]);
	max[1] = (int)std::stof(svec3[1]);
	max[2] = (int)std::stof(svec3[2]);

	std::cout << width << "," << height << "," << depth << std::endl;
	std::cout << min[0] << "," << min[1] << "," << min[2] << std::endl;
	std::cout << max[0] << "," << max[1] << "," << max[2] << std::endl;

	// allocate and write image buffer
	std::map<int, unsigned char*> imgs = std::map<int, unsigned char*>();
	int count = 0;
	for (int z = 0; z < depth; z++)
	{
		float r,g,b;
		unsigned char* image = (unsigned char*)new unsigned char[height * width * BYTES_PER_PIXEL];
		for (int idx = 0; idx < height * width; idx++)
		{
			std::getline(fin, line);
			svec3 = split(line, ',');
			r = std::stof(svec3[0]);
			g = std::stof(svec3[1]);
			b = std::stof(svec3[2]);

			// convert [min ~ max] to [0, 0xFF] 
			// https://en.wikipedia.org/wiki/BMP_file_format, piexl bytes is GBR (i.e. RED is '00 00 FF')
			image[idx*3] = (unsigned char)( 255 * (b - min[0]) / ( max[0] - min[0]));
			image[idx*3+1] = (unsigned char)(255 * (g - min[1]) / (max[1] - min[1]));
			image[idx*3+2] = (unsigned char)(255 * (r - min[2]) / (max[2] - min[2]));
		}
		imgs.insert(std::pair<int, unsigned char*>(z, image));
	}

	// write into bmp file
	int width_block_count = depth > MAX_SLICE_SIZE ? MAX_SLICE_SIZE : depth; // max slice for Texture3D is h:256, v:256
	int widthInBytes = width * width_block_count * BYTES_PER_PIXEL;
	int height_block_count = (depth - 1) / MAX_SLICE_SIZE + 1;
	int remain_block = depth;
	int paddingSize = (4 - (widthInBytes) % 4) % 4;
	int stride = (widthInBytes)+paddingSize;
	
	std::string filename = std::string("t3d_w") + std::to_string(width_block_count) + std::string("_h") + std::to_string(height_block_count) + std::string(".bmp");
	std::ofstream fout(filename, std::ios::binary | std::ios::out );

	unsigned char* fileHeader = createBitmapFileHeader(height * height_block_count, stride);
	fout.write((const char*)fileHeader, FILE_HEADER_SIZE);

	unsigned char* infoHeader = createBitmapInfoHeader(height * height_block_count, width * width_block_count);
	fout.write((const char*)infoHeader, INFO_HEADER_SIZE);

	while (remain_block > 0)
	{
		int start_block_index = depth - remain_block;
		unsigned char* block_line = new unsigned char[height * stride];
		std::memset(block_line, 0, height * stride);
		for (int idx = start_block_index; idx < start_block_index + MAX_SLICE_SIZE; idx++)
		{
			int block_line_idx = depth > MAX_SLICE_SIZE ? start_block_index + MAX_SLICE_SIZE - idx - 1 : idx - start_block_index;
			unsigned char* buffer = imgs[idx];

			if (remain_block <= 0)
				break;

			for (int i = 0; i < height; i++)
			{
				std::memcpy(block_line + i * stride + block_line_idx * width * BYTES_PER_PIXEL, buffer + i * width * BYTES_PER_PIXEL, width * BYTES_PER_PIXEL);
			}

			remain_block = remain_block - 1;
		}
		fout.write((const char*)block_line, height * stride);
		delete[] block_line;
	}

	fout.close();


	// free buffer
	for (const auto& [key, value] : imgs)
	{
		delete[] value;
	}
	imgs.clear();

	return 0;
}

float remap(unsigned char data)
{
	int val = (int)data;
	float fval = (float)val;
	return 512.0 * fval / 255.0 - 256.0;
}

// help function to create BMP file header
unsigned char* createBitmapFileHeader(int height, int stride)
{
	int fileSize = FILE_HEADER_SIZE + INFO_HEADER_SIZE + (stride * height);

	static unsigned char fileHeader[] = {
		0,0,     /// signature
		0,0,0,0, /// image file size in bytes
		0,0,0,0, /// reserved
		0,0,0,0, /// start of pixel array
	};

	fileHeader[0] = (unsigned char)('B');
	fileHeader[1] = (unsigned char)('M');
	fileHeader[2] = (unsigned char)(fileSize);
	fileHeader[3] = (unsigned char)(fileSize >> 8);
	fileHeader[4] = (unsigned char)(fileSize >> 16);
	fileHeader[5] = (unsigned char)(fileSize >> 24);
	fileHeader[10] = (unsigned char)(FILE_HEADER_SIZE + INFO_HEADER_SIZE);

	return fileHeader;
}

// help function to create BMP image header
unsigned char* createBitmapInfoHeader(int height, int width)
{
	static unsigned char infoHeader[] = {
		0,0,0,0, /// header size
		0,0,0,0, /// image width
		0,0,0,0, /// image height
		0,0,     /// number of color planes
		0,0,     /// bits per pixel
		0,0,0,0, /// compression
		0,0,0,0, /// image size
		0,0,0,0, /// horizontal resolution
		0,0,0,0, /// vertical resolution
		0,0,0,0, /// colors in color table
		0,0,0,0, /// important color count
	};

	infoHeader[0] = (unsigned char)(INFO_HEADER_SIZE);
	infoHeader[4] = (unsigned char)(width);
	infoHeader[5] = (unsigned char)(width >> 8);
	infoHeader[6] = (unsigned char)(width >> 16);
	infoHeader[7] = (unsigned char)(width >> 24);
	infoHeader[8] = (unsigned char)(height);
	infoHeader[9] = (unsigned char)(height >> 8);
	infoHeader[10] = (unsigned char)(height >> 16);
	infoHeader[11] = (unsigned char)(height >> 24);
	infoHeader[12] = (unsigned char)(1);
	infoHeader[14] = (unsigned char)(BYTES_PER_PIXEL * 8);

	return infoHeader;
}
