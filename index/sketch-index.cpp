//sketch index
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

#include "../common/record.h"
#include "../common/eval.h"

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	if(argc != 4)
	{
		cout << "Usage: sketch-index <offset> <input> <output> " <<endl;
		return 0;
	};

	PointComparator compare(atoi(argv[1]));

	std::ifstream featin(argv[2],std::ios::binary);
	if(!featin){
		std::cout << "Failed to load feature file :" << argv[2] << std::endl;
		return 0;
	}
	
	
	Timer timer;
	std::cout << "Start to read records from " << argv[2] << std::endl;
	
//	{

	Point point;

	Key maxkey = 0, minkey = 0;
	int cnt = 0;

	std::vector<Point> point_list;
	while(point.readPoint(featin, true))
	{
		point_list.push_back(point);
	
		cnt++;
		
		if(cnt == 1){maxkey = minkey = point.key;}
		else
		{
			if(point.key > maxkey) maxkey = point.key;
			if(point.key < minkey) minkey = point.key;
		}
	}

	featin.close();

	std::cout << point_list.size() << " records imported. " << std::endl;
	std::cout << "Time costs : " << timer.elapsed() << " seconds." << std::endl;
	timer.restart();

 //	}
	std::cout << "Sorting ..." << std::endl;
	std::sort(point_list.begin(), point_list.end(), compare);

	std::cout << "Time costs : " << timer.elapsed() << " seconds." << std::endl;
        timer.restart();

	std::cout << "Writing points ..." << std::endl;

	std::ofstream featout(argv[3],std::ios::binary);
	for(int i=0;i < point_list.size();i++)
	{
		point_list[i].writePoint(featout);
	}

	std::cout << "Time costs : " << timer.elapsed() << " seconds." << std::endl;
        timer.restart();

	std::cout << "Writing conf file ..." << std::endl;
	string conf;
	conf = string(argv[3]) + ".conf";
	ofstream os(conf.c_str());
	os << minkey << "\t" << maxkey << "\t" << point_list.size() << std::endl;
	std::cout << "Finished." << std::endl;
	os.close();

	featout.close();
	
	return 1;
}
