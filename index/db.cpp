//db test
#include <iostream>
#include <fstream>
#include <vector>

#include "db.h"

#include "../common/record.h"
#include "../common/eval.h"

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	if(argc != 3)
	{
		cout << "Usage: db_test <db_config> <feat>" << endl;
		return 0;
	}


	DB db(argv[1]);

	Point pt;
	vector<Key> result;
	
	std::ifstream is(argv[2], std::ios::binary);

	int cnt = 0;

	Timer timer;
	
	while(pt.readPoint(is, true) && cnt < 50)
	{
		db.searchPoint(pt, result);
	
		pt.printPoint();
		for(int i=0;i< result.size();i++)
		{
			cout << result[i] << " ";
		}
		cout << endl;
		cnt++;
		result.clear();
	}
	
	cout << cnt << " points loaded." << endl;
	cout << "Results: " << result.size() << endl;
	cout << "Time costs: " << timer.elapsed() << endl;
	
/*
	SampleIndex index;
	
	ifstream is(argv[1], std::ios::binary);
	
	if(!is)
	{
		cout << "Failed to load " << argv[1] << endl;
		return 0;
	}

	Timer timer;
	
	cout << "Loading ..." << endl;

	int size = index.load(is);

	cout << size << " records loaded." << endl;
	cout << "Time costs: " << timer.elapsed() << " seconds" << endl;	
	
	Point pt;
	Range range;

	cout << "Testing ..." << endl;

	timer.restart();
	for(int i = 0;i < 1000; i++)
		index.search(pt, range);
	
	cout << "Time costs: " << timer.elapsed() << " seconds" << endl;
*/
	return 1;
};
