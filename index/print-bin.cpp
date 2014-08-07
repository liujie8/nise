//print bin

#include "../common/bin.h"

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	if(argc != 2)
	{
		cout << "Usage: print-bin <bin file>" << endl;
		return 0;
	};

	ifstream is(argv[1], std::ios::binary);
	
	if(!is)
	{
		cout << "Failed to load " << argv[1] << endl;
		return 0;
	};

	Bin bin;

	while(bin.readBin(is))
	{
		bin.printBin();
	};
	
	return 1;
};
