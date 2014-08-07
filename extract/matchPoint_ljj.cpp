#include <vector>
#include <string>

#include "../common/record.h"
#include "../common/eval.h"
#include "../common/hamming.h"
#include "../common/key.h"
#include "../common/record.h"
#include "../common/hamming.h"
#include "feature_extractor_test.h"

using namespace std;
using namespace nise;

struct Point
{
	int x;
	int y;
	Point(int x_ = 0, int y_ = 0)
	{
		x = x_;
		y = y_;
	}
};

int main(int argc, char** argv)
{
	FeatureExtractor xtor;
        Record record;
        std::vector<Key> result[10];
        std::string res[10];
        if(xtor.loadImage(argv[1]) && xtor.loadImage(argv[2]))
       	{
   	//       xtor.getBitDesc(record.desc, LDAHASH, false);
		std::vector<Feature> feat1, feat2;
		std::vector<SiftFeature> desc1, desc2;
		xtor.extractSiftDesc(&feat1);
		xtor.extractSiftDesc(&feat2);
desc1.resize(feat1.size());

                        if(feat1.size()==0)return 0;

                        for(int i=0;i< feat1.size();i++)
                                for(int j=0;j< SIFT_DIM; j++)
                                        desc1[i].sketch[j] = feat1[i].desc[j];
desc2.resize(feat2.size());

                        if(feat2.size()==0)return 0;

                        for(int i=0;i< feat2.size();i++)
                                for(int j=0;j< SIFT_DIM; j++)
                                        desc2[i].sketch[j] = feat2[i].desc[j];

		for (int i=0; i<feat1.size(); i++)
		{
			for (int j=0; j<feat2.size(); j++)
			{
				Hamming hamming;
				if ((int)(hamming((Chunk*)desc1[i].sketch, (Chunk*)desc2[j].sketch)) <= 3)
				{
					
				}				
			}
		}
        }
        else
                std::cout << "Failed to load image " << std::endl;
	return 0;
}
