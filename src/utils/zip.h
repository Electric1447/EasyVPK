#pragma once

#include <stdio.h>
#include <string.h>
#include <string>
#include <stdexcept>

#include "minizip/unzip.h"


using namespace std;


class Zipfile {
public:
	Zipfile(const string zip_path);
	~Zipfile();

	int Unzip(const string outpath);
	int UncompressedSize();

private:
	unzFile zipfile_;
	uint64_t uncompressed_size_ = 0;
	unz_global_info global_info_;
};
