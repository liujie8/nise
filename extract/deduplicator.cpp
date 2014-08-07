//This file is used to extract fingerprint from input images
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <queue>

//boost
#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/thread/condition.hpp>

//common
#include "../common/eval.h"
#include "../common/key.h"

using namespace std;
using namespace nise;

struct KeySet
{
	std::queue<Key> keyset;
	
	void push(Key key)
	{
		keyset.push(key);
	};

	void pop()
	{
		keyset.pop();
	};

	Key top()
	{
		return keyset.front();
	};
	
	int size()
	{
		return keyset.size();
	};
};

//feature extraction thread
//static const int THREAD_NUM = 8;
static int cnt = 0;
static int total = 0;
static int image_num = 0;
static unsigned long long fg = 0;
boost::mutex io_mutex;
std::ofstream os("url.fg");
std::map<unsigned long long, int> finger;

bool checkFingerPrint(unsigned long long fingerprint)
{
	std::map<unsigned long long, int>::iterator iter;

	iter = finger.find(fingerprint);
	if(iter != finger.end())
	{
		iter->second++;
		return true;
	}
	else
	{
		finger.insert(std::make_pair(fingerprint, 1));
		return false;
	}	
};

int save(std::ofstream &os)
{
	std::map<unsigned long long, int>::iterator iter;
	
	int cnt = 0;
	for(iter = finger.begin();iter != finger.end();iter++)
	{
		os << iter->first << "\t"<< iter->second <<std::endl;
		
		if(os)cnt++;
	}
	return cnt;
};

void extract(std::vector<std::string> &img_urls, std::ofstream *out, KeyDistributor *key_dis)
{
	{
	   boost::mutex::scoped_lock lock(io_mutex);       //lock and enter critical section 
	   std::cout << "Start to extract image features..." << std::endl;
	}

	FeatureExtractor feat;  //feature extractor
	
	int num=0;

        BOOST_FOREACH(std::string &img_path, img_urls)
        {
                if(!feat.loadImage(img_path))
		{
			boost::mutex::scoped_lock lock(io_mutex);       //lock and enter critical section
                        std::cout<< "Failed to load image: "<< img_path.c_str()<< std::endl;
               	}
		else
                {
		//	{
		//		boost::mutex::scoped_lock lock(io_mutex); 
		//		std::cout << img_path << std::endl;
		//	}

			unsigned long long fingerprint = feat.getFingerPrint();

			boost::mutex::scoped_lock lock(io_mutex);	//lock and enter critical section		
			
			total++;
		
			bool exist = checkFingerPrint(fingerprint);
			
			if(!exist)cnt++;

//			if(fingerprint == fg) *out << img_path << endl;

			std::cout << img_path << std::endl;
			std::cout << "Finger Print :" << fingerprint << std::endl;
			std::cout << "Total image      current      deduplicate        percentage " << std::endl;
			std::cout << image_num << "\t" << total << "\t" << cnt << "\t" << (float)(cnt)/total << std::endl;

                }
	}
	return;
};

int main(int argc,char **argv)
{
	if(argc != 3)
	{
		cout << "Usage: deduplicate <image path> <result>" << endl;
		return 0;
	}

	finger.clear();

	KeyDistributor key_dis(0);

	boost::thread * thread[THREAD_NUM];

	int thread_count = 0;
	for(int i=0;i < THREAD_NUM; i++)
	{
		thread[i] = (boost::thread *)new boost::thread(boost::bind(extract,urls[i], &os, &key_dis));
		if(thread[i]==NULL)
		{
			boost::mutex::scoped_lock lock(io_mutex);
			std::cout << "Failed to create thread " << i << std::endl;
		}
		else
			thread_count++;
	}
	
	{
		boost::mutex::scoped_lock lock(io_mutex);
		std::cout << thread_count << " threads started." << std::endl;
	}

	//wait until thread finished
	for(int i=0;i < THREAD_NUM; i++)
	{
		(*thread[i]).join();
		if(thread[i])
			delete thread[i];
		boost::mutex::scoped_lock lock(io_mutex);
		std::cout << "Thread " << i << " joined."<< std::endl;
	}
	
	os.close();
	std::cout << "Extract image features from " << cnt << " images successfully!" << std::endl;
	std::cout << "Total time used: " << timer.elapsed() << " seconds." << std::endl;

//	cout << "Match fingerprint : " << fg << endl;
//	std::cout << "Saving fingerprint..." << std::endl;

	ofstream out(argv[2]);
	int n = save(out);

	std::cout << n << " fingerprints saved." << std::endl;

	return 1;
}
