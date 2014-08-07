//add key
#include <iostream>
#include <fstream>
#include <stdlib.h>
//#include <stdio.h>

#include "../common/record.h"
#include "../common/eval.h"

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	if(argc != 4)
	{
		cout << "Usage : addkey <Key> <index> <output>" << endl;
		return 0;
	}

	Point point;
        Key key = atoi(argv[1]);
	std::ifstream is(argv[2], std::ios::binary);
	std::ofstream os(argv[3], std::ios::binary);
	
	if(!is || !os)
	{
		cout << "Failed to open index file." << endl;
		return 0;
	}
	
	Timer timer;
	cout << "Start to check ..." << endl;

	int cnt = 0;

	while(point.readPoint(is, true))
	{
		point.key += key;
		point.writePoint(os, true);
		cnt++;
	}
	cout << "Finished." << endl;
	cout << "Add key "<< key <<" to " << cnt << " points ." << endl;
	cout << "Time costs: " << timer.elapsed() << " seconds. " << endl;

	is.close();
	os.close();
	return 1;
};
