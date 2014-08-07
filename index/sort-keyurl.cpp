//add key to key_url
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

#include <stdlib.h>
//#include <stdio.h>
#include "../common/config.h"
#include "../common/eval.h"

using namespace std;
using namespace nise;

struct KeyUrl
{
	Key k;
	int num;
	unsigned long long fingerprint;
	std::string img_url;

	bool readKeyUrl(std::ifstream &is)
	{
		return is >> k >> num >> fingerprint >> img_url;
	};
	
	bool writeKeyUrl(std::ofstream &os)
	{
		return os << k << "\t" << num << "\t" << fingerprint << "\t" << img_url << endl;
	};

	bool operator () (const KeyUrl& k1, const KeyUrl &k2)
	{
		if(k1.fingerprint < k2.fingerprint)return true;
		if(k1.fingerprint > k2.fingerprint)return false;
		if(k1.k < k2.k)return true;
		else return false;
	};
};
int main(int argc, char ** argv)
{
	if(argc != 3)
	{
		cout << "Usage : sort-keyurl <key_url> <output>" << endl;
		return 0;
	}

	std::ifstream is(argv[1], std::ios::in);
	std::ofstream os(argv[2], std::ios::out);
	
	if(!is || !os)
	{
		cout << "Failed to open key_url file." << endl;
		return 0;
	}

	Timer timer;
	cout << "Start to check ..." << endl;

	int cnt = 0;

	std::vector<KeyUrl> keyurls;
	KeyUrl keyurl,cmp;

	keyurls.clear();

	while(keyurl.readKeyUrl(is))
	{
		keyurls.push_back(keyurl);
		cnt++;
	}

	cout << cnt << " key url maps imported." << endl;
	cout << "Time costs: " << timer.elapsed() << " seconds. " << endl;
	timer.restart();

	cout << "Sorting by fingerprint ..." << endl;

	std::sort(keyurls.begin(), keyurls.end(), cmp);

	cout << "Sorting Finished." << endl;
	cout << "Time costs: " << timer.elapsed() << " seconds. " << endl;
	timer.restart();

	cout << "Writing results ..." << endl;

	for(int i=0;i< keyurls.size();i++)
	{
		keyurls[i].writeKeyUrl(os);
	}

	cout << "Writing Finished." << endl;
	cout << "Time costs: " << timer.elapsed() << " seconds. " << endl;

	is.close();
	os.close();
	return 1;
};
