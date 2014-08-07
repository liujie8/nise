//print-keypid
#include <iostream>
#include <fstream>

#include "../common/key.h"

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	if(argc != 2)
	{
		cout << "Usage : print-keypid <keypid>" << endl;
		return 0;
	}

	ifstream is(argv[1], std::ios::binary);
	
	if(!is)
	{
		cout << "Failed to load " << argv[1] << endl;
		return 0;
	}
	
	KeyPair keypair;

	while(keypair.readPair(is))
	{
		keypair.printPair();
	};

	is.close();
	
	return 1;
	
};
