#include <vector>

#include "../main.hpp"


#define NET_INIT_SIZE 1*1024*1024
#define CHUNK_MAXSIZE 32*1024*1024


#define VPK         0
#define DATA        1

#define IDLE        0
#define DOWNLOADING 1
#define DOWNLOADED  2
#define EXTRACTING  3
#define FINISHED    4
#define MISSING     5
#define ERROR       6


class Installer {
	public:
		void draw(SharedData &sharedData);
		void reset();

	private:
		string getDataFileName(const string& s);
		vector<string> installFiles;
};