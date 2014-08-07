//define the record struct for an image
#ifndef _COMMON_RAWRECORD
#define _COMMON_RAWRECORD

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <bitset>

#include "io.h"
#include "config.h"

namespace nise
{
//	typedef std::bitset<DESC_LEN> Feature;	
//	typedef unsigned long long Sketch[BLOCK_SIZE];
	struct SiftFeature
	{
		float sketch[SIFT_DIM];
	};

	struct RawRecord
	{

		
		std::string img_url;   //image url
		
		std::vector<SiftFeature> desc;  //image descriptors

		RawRecord(){};
		~RawRecord(){};

		void readFields(std::ifstream &is)
		{
			img_url.clear();
			desc.clear();

			//read img_url
			size_t url_sz=0;
			is.read((char *)&url_sz,sizeof(size_t));
			img_url.resize(url_sz);
	//		std::cout <<"url_sz:"<< url_sz << std::endl;
			if(url_sz>0)
				is.read((char *)&img_url.at(0),url_sz);
			
			//read descriptors
			size_t desc_sz=0;
			is.read((char *)&desc_sz,sizeof(size_t));
			desc.resize(desc_sz);
	//		std::cout << "desc_sz:" << desc_sz << std::endl;
			if(desc_sz>0)
				is.read(reinterpret_cast<char *>(&desc.at(0)), desc_sz * sizeof(desc.at(0)));
		};

		void writeFields(std::ofstream &os)
		{
			//write img_url
			size_t url_sz = img_url.size();
			os.write((char *)&url_sz,sizeof(size_t));
			if(url_sz>0)
				os.write(&img_url[0],url_sz);

			//write descriptors
			size_t desc_sz = desc.size();
			os.write((char *)&desc_sz,sizeof(size_t));
			if(desc_sz>0)
				os.write(reinterpret_cast<const char *>(&desc[0]), desc_sz * sizeof(desc[0]));
		};	

	};
}
#endif
