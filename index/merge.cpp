//merge index files
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>

#include "../common/record.h"
#include "../common/eval.h"

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	if(argc != 5)
	{
		cout << "Usage: merge <offset> <index1> <index2> <output>" << endl;
		return 0; 
	}
	
	PointComparator compare(atoi(argv[1]));
	
	std::ifstream is1(argv[2], std::ios::binary);
	std::ifstream is2(argv[3], std::ios::binary);
	std::ofstream os(argv[4], std::ios::binary);

	if(!is1 || !is2)
	{
		cout << "Failed to load index file." << endl;
		return 0;
	}
	
	Timer timer;

	std::cout << "Merging ..." << std::endl;

	Point p1,p2;

	p1.readPoint(is1, true);
	p2.readPoint(is2, true);
	
	while(is1 && is2)
	{
		if(compare(p1,p2)) //if p1 < p2 return true
		{
			p1.writePoint(os, true);
			p1.readPoint(is1, true);
		}
		else
		{
			p2.writePoint(os, true);
			p2.readPoint(is2, true);
		}
	};
	
	while(is1)
	{
		 p1.writePoint(os, true);
                 p1.readPoint(is1, true);
	};

	while(is2)
	{
		p2.writePoint(os, true);
                p2.readPoint(is2, true);
	};
	
	std::cout << "Time costs: " << timer.elapsed() << " seconds." << std::endl;

	is1.close();
	is2.close();
	os.close();
	return 1;	
};
