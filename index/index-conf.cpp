//index conf
#include <iostream>
#include <fstream>
#include <string>

#include "../common/record.h"
#include "../common/eval.h"

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	if(argc != 3)
	{
		cout << "Usage: index-conf <index> <index-conf> " <<endl;
		return 0;
	};

	std::ifstream is(argv[1],std::ios::binary);
	if(!is){
		std::cout << "Failed to load index file :" << argv[1] << std::endl;
		return 0;
	}
	
	Timer timer;
	std::cout << "Start to read records from " << argv[1] << std::endl;

	Point point;
	Key maxkey = 0, minkey = 0;
	int cnt = 0;

	while(point.readPoint(is, true))
	{
		cnt++;
		
		if(cnt == 1){maxkey = minkey = point.key;}
		else
		{
			if(point.key > maxkey) maxkey = point.key;
			if(point.key < minkey) minkey = point.key;
		}
	}

	is.close();

	std::cout << cnt << " records imported. " << std::endl;
	std::cout << "Minkey : " << minkey << "\t Maxkey : " << maxkey << std::endl;
	std::cout << "Time costs : " << timer.elapsed() << " seconds." << std::endl;
	timer.restart();

	std::cout << "Writing conf file ..." << std::endl;
	ofstream os(argv[2]);
	os << minkey << "\t" << maxkey << "\t" << cnt << std::endl;
	std::cout << "Finished." << std::endl;
	os.close();
	
	return 1;
}
