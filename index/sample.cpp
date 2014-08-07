//sample
#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "../common/record.h"
#include "../common/eval.h"

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	if(argc != 4)
	{
		cout << "Usage: sample <sample_rate> <index> <output>" << endl;
		return 0;
	}
	
	ifstream is(argv[2], std::ios::binary);
	ofstream os(argv[3], std::ios::binary);

	if(!is || !os)
	{
		cout << "Failed to open file." << endl;
		return 0;
	}
	
	unsigned record_size = Point::size();

	unsigned sample_rate = atoi(argv[1]);
	unsigned skip_size = record_size * (sample_rate - 1);
	
	Timer timer;
	Point pt;
	
	cout << "Sampling ..." << endl;

	while(pt.readPoint(is, true))
	{
		pt.writePoint(os, true);
		is.seekg(skip_size, std::ios::cur);
	}
	cout << "Finished !" << endl;
	cout << "Time cost: " << timer.elapsed() << " seconds." << endl;

	is.close();
	os.close();
	
	return 1;

};
