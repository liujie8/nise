//This file is used to extract fingerprint from input images
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

//opencv
#include <opencv/cv.h>
#include <opencv/highgui.h>
//boost
#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/thread/condition.hpp>

//common
#include "../common/config.h"
#include "../common/record.h"
#include "../common/keypoint.h"
#include "../common/eval.h"
#include "../common/key.h"

//feature extractor
//#include "feature_extractor.h"

using namespace std;
using namespace nise;

//get all the file urls under path directory
void getFiles( std::string path, std::vector<std::string>& files )
{
	DIR    *dir;
	char    fullpath[1024],currfile[1024];
	struct    dirent    *s_dir;
	struct    stat    file_stat;
	strcpy(fullpath, path.c_str());
	dir = opendir(fullpath);
	if(dir == NULL)
	{
		printf("Failed to open %s.\n", fullpath);
		return;
	}
	while((s_dir = readdir(dir))!=NULL){
		if((strcmp(s_dir->d_name,".")==0)||(strcmp(s_dir->d_name,"..")==0))
			continue;
		sprintf(currfile,"%s/%s",fullpath,s_dir->d_name);
		stat(currfile,&file_stat);
		if(S_ISDIR(file_stat.st_mode))
			getFiles(std::string(currfile),files);
		else
			files.push_back(std::string(currfile));
	}
	closedir(dir);
};

struct Urls
{
	std::vector<std::string> urls;

	Urls(){urls.clear();};
	~Urls(){};
};

//feature extraction thread
//static const int THREAD_NUM = 8;
static int cnt = 0;
static int total = 0;
static int image_num = 0;
static unsigned long long fg = 0;
boost::mutex io_mutex;
std::ofstream os("url.fg");
std::map<unsigned long long, Urls> finger;

bool checkFingerPrint(unsigned long long fingerprint, std::string &url)
{
	std::map<unsigned long long, Urls>::iterator iter;

	iter = finger.find(fingerprint);
	if(iter != finger.end())
	{
		iter->second.urls.push_back(url);
		return true;
	}
	else
	{
		Urls path;
		path.urls.push_back(url);

		finger.insert(std::make_pair(fingerprint, path));
		return false;
	}	
};

int save(std::ofstream &os)
{
	std::map<unsigned long long, Urls>::iterator iter;

	int cnt = 0;
	for(iter = finger.begin();iter != finger.end();iter++)
	{
		int size = iter->second.urls.size();
		os << iter->first << "\t"<< size <<std::endl;

		for(int i=0; i<size; i++)
			os << iter->second.urls[i] << std::endl;
		os << std::endl;

		if(os)cnt++;
	}
	return cnt;
};

unsigned long long getFingerPrint(IplImage * img)
{
        //get a 64 bits fingerprint of the image

        if(!img)return 0;

        CvSize size;
        size.height = 12;
        size.width = 8;

        if(img->nChannels != 1)return 0;

        if(img->width * img->height <= 0)return 0;

        IplImage * fingerImage = cvCreateImage(size, img->depth, 1);
        if(fingerImage == NULL)return 0;

        cvResize(img, fingerImage, CV_INTER_AREA);

        float mean=0,sum=0;
        unsigned long long fingerprint = 0;
        unsigned long long mask[64];

        for(int n=0;n<64;n++)
                mask[n] = 1 << n;

        CvScalar s;
        for(int i=0;i<8;i++)
                for(int j=0;j<8;j++)
                {
                        s = cvGet2D(fingerImage, i,j);
                        sum += s.val[0];
                }

        mean = sum/64;

	for(int i=0;i<8;i++)
	{
		if(i<7)
		{
			for(int j=0;j<8;j++)
			{
				s = cvGet2D(fingerImage, i,j);
				//              cout << s.val[0] << "\t" ;
				if( s.val[0] >= mean)
					fingerprint = fingerprint ^ mask[i*8+j];
			}
		}
		else
		{
			unsigned char m = (unsigned char)mean;

			for(int n=0;n<8;n++)
				if( (m >> n) & 1 != 0)
					fingerprint = fingerprint ^ mask[i*8+n];
		}
		/*
		   for(int j=0;j<8;j++)
		   {
		   s = cvGet2D(fingerImage, i,j);
		   if( s.val[0] >= mean)
		   fingerprint = fingerprint ^ mask[i*8+j];
		   }
		 */
	}

        cvReleaseImage(&fingerImage);

        return fingerprint;
};

void extract(std::vector<std::string> &img_urls, std::ofstream *out, KeyDistributor *key_dis)
{
	{
	   boost::mutex::scoped_lock lock(io_mutex);       //lock and enter critical section 
	   std::cout << "Start to extract image features..." << std::endl;
	}

//	FeatureExtractor feat;  //feature extractor
	
	int num=0;

        BOOST_FOREACH(std::string &img_path, img_urls)
        {
		IplImage *img = cvLoadImage(img_path.c_str(),CV_LOAD_IMAGE_GRAYSCALE);
                if(!img)
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

			unsigned long long fingerprint = getFingerPrint(img);

			boost::mutex::scoped_lock lock(io_mutex);	//lock and enter critical section		
			total++;
		
			bool exist = checkFingerPrint(fingerprint, img_path);
			
			if(!exist)cnt++;
			
			cvReleaseImage(&img); //release image;
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

//	fg = atoll(argv[3]);
//	cout << "Match fingerprint:" << fg << endl;

	std::string init_path("/data1/liujie/work/nise/image");  //default image path
	
	init_path = std::string(argv[1]);

	std::vector<std::string> img_urls;  //all image urls

	std::cout << "Start get image urls..." << std::endl;
	getFiles(init_path, img_urls);    // get all the image urls under path directory
	for(int i=0;i < img_urls.size();i++)
		std::cout << img_urls[i] << std::endl;

	std::cout << img_urls.size() << " images imported. " << std::endl;
	
	if(img_urls.size()==0)return 0;
	
	std::cout << "Start extract features from images..." << std::endl;

	std::string feat_path("raw_feat"); 
	std::ofstream os(feat_path.c_str(), std::ios::binary);
	if(!os)
	{
		std::cout <<  "Failed to create file " << feat_path << std::endl;
		return 0;
	}

	//feature extraction	
//	if(img_urls.size()>100000)
//		img_urls.resize(100000);
	//partition of all img_urls

	finger.clear();

	image_num = img_urls.size();

	std::vector<std::string> urls[THREAD_NUM];
	for(int i=0; i< img_urls.size();i++)
	{
		int j = i * THREAD_NUM /img_urls.size();
		urls[j].push_back(img_urls[i]);
	};

	Timer timer; // start timing

/*
	std::cout << urls[0].size()  << std::endl;
	for(int i=0;i< urls[0].size();i++)
		std::cout << urls[0][i] << std::endl;
	extract(urls[0],os);
*/
	//bind to thread

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
