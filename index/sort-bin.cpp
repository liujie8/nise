//sort bin
#include <vector>
#include <algorithm>

#include "../common/bin.h"
#include "../common/eval.h"

using namespace std;
using namespace nise;

bool compare_bin(Bin &b1, Bin &b2)
{
	return b1.count > b2.count;
}


int main(int argc, char ** argv)
{
	if(argc != 3)
	{
		cout << "Usage: sort-bin <bin file> <output>" << endl;
		return 0;
	};

	ifstream is(argv[1], std::ios::binary);
	
	if(!is)
	{
		cout << "Failed to load " << argv[1] << endl;
		return 0;
	};

	Bin bin;
	vector<Bin> bin_vec;
	bin_vec.clear();

	int cnt =0;
	int total =0;

	Timer timer;

	cout << "reading bin file ..." << endl;
	while(bin.readBin(is))
	{
		cnt++;
		total += bin.count;

		bin_vec.push_back(bin);
	};

	cout << cnt << " bins imported." << endl;
	cout << "total size : " << total << endl;

	cout << "Time costs : " << timer.elapsed() << " seconds." << endl;
	timer.restart();
	cout << "sorting by count size..." << endl;

	std::sort(bin_vec.begin(), bin_vec.end(), bin);
	
	cout << "Time costs : " << timer.elapsed() << " seconds." << endl;
	timer.restart();

	ofstream os(argv[2],std::ios::binary);

	if(!os)
	{
		cout << "Failed to load " << argv[2] << endl;
		return 0;
	};
	
	cout << "writing results ..." << endl;

	for(int i=0;i< bin_vec.size();i++)
		bin_vec[i].writeBin(os);
	cout << "Time costs : " << timer.elapsed() << " seconds." << endl;
	return 1;
};
