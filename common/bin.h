#ifndef _COMMON_BIN
#define _COMMON_BIN

#include <iostream>
#include <fstream>

namespace nise
{
	struct Bin
	{
		unsigned int key;
		unsigned int count;

		Bin(){};
		~Bin(){};

		void printBin()
		{
			std::cout << key << "\t" << count << std::endl;
		};

		bool readBin(std::ifstream &is)
		{
			is.read((char *)&key, sizeof(unsigned int));
			is.read((char *)&count, sizeof(unsigned int));

			if(is)
				return true;
			else
				return false;
		};

		bool writeBin(std::ofstream &os)
		{
			os.write((char *)&key, sizeof(unsigned int));
			os.write((char *)&count, sizeof(unsigned int));

			if(os)
				return true;
			else
				return false;
		};

		bool operator() (const Bin &b1, const Bin &b2)
		{
			return b1.count > b2.count;
		};
	};
};

#endif
