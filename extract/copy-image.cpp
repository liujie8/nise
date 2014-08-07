//copy image

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

int main(int argc, char **argv)
{
	if(argc != 3)
	{
		cout << "Usage: copy-image <input file> <output dir>" << endl;
		return 0;
	}
	ifstream is(argv[1]);

	unsigned long long fpkey;
	int size;
	string dir;

	while(is >> fpkey >> size)
	{
		std::stringstream cmd;

		cmd << "mkdir -p " << argv[2] << "//"<< fpkey ;
		cout << cmd.str()<< endl;

		system(cmd.str().c_str());

		getline(is,dir);

		for(int i=0;i<size;i++)
		{	
			std::stringstream c;
			if(getline(is,dir))c << "cp " << "\""<<dir << "\""<<" "<<"\""<<argv[2] << "/" << fpkey<< "\"";

			cout << c.str() << endl;
			system(c.str().c_str());
		}
		cout << endl;
	};
	return 1;
};
