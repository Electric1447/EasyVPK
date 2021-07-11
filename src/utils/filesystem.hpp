#pragma once

#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <string>


#define SCE_ERROR_ERRNO_EEXIST 0x80010011
#define MAX_PATH_LENGTH 1024

#define TRANSFER_SIZE (128 * 1024)


using namespace std;


class Filesystem {
	public:
		static string readFile(string file);
		static int writeFile(string file, string buf);
		static int copyFile(string src, string dst);
		static int copyPath(string src, string dst);
		static int mkDir(string path);
		static int removePath(string path);
		static bool fileExists(string path);
};

int doesDirExist(const char* path);

int copyFile(const char *src_path, const char *dst_path);