//add key to key_url
#include <iostream>
#include <string>
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
		cout << "Usage : addkey <Key> <key_url> <output>" << endl;
		return 0;
	}

	Point point;
        Key key = atoi(argv[1]);
	std::ifstream is(argv[2], std::ios::in);
	std::ofstream os(argv[3], std::ios::out);
	
	if(!is || !os)
	{
		cout << "Failed to open key_url file." << endl;
		return 0;
	}
	
	Timer timer;
	cout << "Start to check ..." << endl;

	int cnt = 0;

	Key k;
	int num;
	unsigned long long fingerprint;
	string img_url;

	while(is >> k >> num >> fingerprint >> img_url)
	{
		k = k + key;
		os << k << "\t" << num << "\t" << fingerprint << "\t" << img_url << endl;
		cnt++;
	}
	cout << "Finished." << endl;
	cout << "Add key "<< key <<" to " << cnt << " key url maps ." << endl;
	cout << "Time costs: " << timer.elapsed() << " seconds. " << endl;

	is.close();
	os.close();
	return 1;
};
