#ifndef _COMMOM_CONFIG
#define _COMMOM_CONFIG
//global pamameters used in this project

namespace nise
{
        typedef unsigned char Chunk; // could be unsigned short or unsigned.
        typedef unsigned Key;        // any plain old data types
        static const unsigned DATA_BIT = 128; // can be divided by CHUNK_BIT

	typedef unsigned SketchID;  //sketch id

        static const unsigned CHUNK_BIT = sizeof(Chunk) * 8;
        static const unsigned DATA_CHUNK = 128 / CHUNK_BIT;
        static const unsigned DATA_SIZE = DATA_CHUNK * sizeof(Chunk);
        static const unsigned KEY_SIZE = sizeof(Key);
        static const unsigned RECORD_SIZE = DATA_SIZE + KEY_SIZE;
        static const unsigned MAX_SCAN_RESULT = 500;

	static const int DESC_LEN = 128;   //descriptor length
	static const int BLOCK_SIZE = 4; //block size
	static const int MAX_FEATURES = 100;  //max features
	static const int SIFT_DIM =128;  //sift dimension

	//feature extraction
	static const int MIN_IMAGE_SIZE = 50;
	static const int MAX_IMAGE_SIZE = 120;
	static const int MIN_IMAGE_RATIO = 4;
	static const int BASE_IMAGE_SIZE = 100;
	static const float MIN_ENTROPY = 4.4F;
	static const float LOG_BASE = 0.001F;
	
	//feature matching
	static const int HAMMING_THRESHOLD = 3;
	static const int KEYPOINT_THRESHOLD = 1;

	//sketcher file
	static const char SKETCHER[30] = "sketcher.dat";
	
	//thread num
	static const int THREAD_NUM = 8;
}

#endif

