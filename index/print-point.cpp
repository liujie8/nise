//print point
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>

#include "../common/record.h"
#include "../common/eval.h"

using namespace std;
using namespace nise;

static inline void PrintPoint (const unsigned char *data, unsigned key) 
{       
	for (unsigned i = 0; i < 128 / 8; ++i) 
	{           
		 printf("%02X", data[i]);       
	}  
	printf("       %u", key); 
	printf("\n");  
};

int main(int argc, char ** argv)
{
	if(argc != 2)
	{
		cout << "Usage: print-point <feat>" <<endl;
		return 0;
	};

	std::ifstream featin(argv[1],std::ios::binary);
	if(!featin){
		std::cout << "Failed to load feature file :" << argv[1] << std::endl;
		return 0;
	}
	
	Point pt;
	while(pt.readPoint(featin))
	{
		pt.printPoint();
	};

	featin.close();
/*	
	std::vector<Point> point_list;
	
	Timer timer;
	std::cout << "Start to read records from " << argv[1] << std::endl;

	Point point;
	while(1){
		point.readPoint(featin, true);
		if(featin)
			point_list.push_back(point);
		else
			break;
	}

	featin.close();

	std::cout << point_list.size() << " records imported. " << std::endl;
	std::cout << "Time costs : " << timer.elapsed() << " seconds." << std::endl;
	timer.restart();
 
	for(int i=0;i < point_list.size();i++)
	{
		unsigned char * point = (unsigned char *) point_list[i].desc.sketch;
		unsigned key = point_list[i].key;
		PrintPoint(point, key);
	}
*/	
	return 1;
}
