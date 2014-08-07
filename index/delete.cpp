//delete pid
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
		cout << "Usage : delete <Key> <index> <output>" << endl;
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

	while(1)
	{
		point.readPoint(is, true);
		if(is)
		{
			if(point.key >= key)
			{
				point.key -= key;
				point.writePoint(os, true);
			}
			else
				cnt++;
		}
		else
			break;
	}
	cout << "Finished." << endl;
	cout << cnt << " points deleted." << endl;
	cout << "Time costs: " << timer.elapsed() << " seconds. " << endl;

	is.close();
	os.close();
	return 1;
};
