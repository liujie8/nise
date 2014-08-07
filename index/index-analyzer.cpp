//index analyzer

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <map>

#include "../common/record.h"
#include "../common/eval.h"
#include "../common/bin.h"

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	if(argc != 4)
	{
		cout << "Usage: index-analyzer <offset> <index> <bin file>" << endl;
		return 0;
	}

	ifstream is(argv[2], std::ios::binary);
	ofstream os(argv[3], std::ios::binary);

	if(!is || !os)
	{
		cout << "Failed to open index file. " << endl;
		return 0;
	}

	Point pt;
	Bin bin;
	map<unsigned int, unsigned int> bin_stat;
	map<unsigned int, unsigned int>::iterator iter;

	unsigned int * key = (unsigned int *)pt.desc.sketch;
	int offset = atoi(argv[1]);
	
	offset /= 32;

	cout << "analyzing index ..." << endl;

	int cnt=0;

	Timer timer;
	
	while(pt.readPoint(is, true))
	{
		cnt++;

		iter = bin_stat.find(key[offset]);
		if( iter != bin_stat.end())
			iter->second++;
		else
			bin_stat.insert(make_pair(key[offset], 1));
	};
	
	cout << "analyze finished !" << endl;
	cout << cnt << " points analyzed." << endl;
	cout << "Time costs : " << timer.elapsed() << " seconds."<<endl;
	
	timer.restart();
	cout << "writing results ..." << endl;
	
	for(iter = bin_stat.begin();iter != bin_stat.end(); iter++)
	{
		bin.key = iter->first;
		bin.count = iter->second;
		
		bin.writeBin(os);
	}

	cout << "finished!" << endl;
	cout << "Time costs : " << timer.elapsed() << " seconds." << endl;
	return 1;
	
};
