//basic struct for image metadata
#ifndef _COMMON_IMAGECONTENT
#define _COMMON_IMAGECONTENT

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <queue>

#include <boost/thread/mutex.hpp>

#include "key.h"

namespace nise
{
	class ImageContent
	{
		Key key; //unique image key
		std::string pid; //image pid
		std::string indextime; //image index time
		std::string indexserver //image index server
		unsigned long long fingerprint; //image fingerprint
		int point_sz; //sift point number
		int width; //image width
		int height; //image height
		int file_sz; //file size
	};
};

#endif
