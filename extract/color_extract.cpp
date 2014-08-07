//This file is used to extract feature descriptors from input images
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

//image loader
#include "../common/imageloader.h"

//color feature extractor
#include "../common/color.h"

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

//feature extraction thread
//static const int THREAD_NUM = 8;
static int cnt = 0;
boost::mutex io_mutex;
std::ofstream os("key_url");

void extract(std::vector<std::string> &img_urls, std::ofstream *out, KeyDistributor *key_dis)
{
	{
	   boost::mutex::scoped_lock lock(io_mutex);       //lock and enter critical section 
	   std::cout << "Start to extract image features..." << std::endl;
	}

	ImageLoader loader;
	ColorExtractor feat;  //feature extractor
	
	int num=0;

        BOOST_FOREACH(std::string &img_path, img_urls)
        {
                if(!loader.loadImage(img_path))
		{
			boost::mutex::scoped_lock lock(io_mutex);       //lock and enter critical section
                        std::cout<< "Failed to load image: "<< img_path.c_str()<< std::endl;
               	}
		else
                {
                        ColorFeature record;  //image record to store image features and image url
                        record.img_url = img_path;
			
			Point point;

                        int num = feat.get_HS_Hist(loader.getImg(), feat.);


			boost::mutex::scoped_lock lock(io_mutex);	//lock and enter critical section		
                        std::cout << "Image: " << img_path << std::endl;
                        std::cout << "width: " << loader.getImg()->width << " ";
                        std::cout << "height: " << loader.getImg()->height << std::endl;
                        std::cout << "Desc extracted: " << num << std::endl;
			
			point.key = key_dis->distributeKey();

			os << point.key << "   "<< num << "   " << fingerprint<< "   "<< img_path << std::endl;

			for(int i=0;i < record.desc.size();i++)
			{
				point.desc = record.desc[i];
				point.writePoint(*out, true);
			}
//                        record.writeFields(*out);
                        cnt++;
                }
	}
	return;
};

int main(int argc,char **argv)
{
	if(argc !=3)
	{
		cout << "Usage: extract <image path> <init key>" << endl;
		return 0;
	}

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
	std::vector<std::string> urls[THREAD_NUM];
	for(int i=0; i< img_urls.size();i++)
	{
		int j = i * THREAD_NUM /img_urls.size();
		urls[j].push_back(img_urls[i]);
	};

/*
	{
		//if sketcher not exist, create a sketcher
		std::ifstream in(SKETCHER, std::ios::binary);
		if(!in)
		{
			FeatureExtractor feat;
			std::ofstream sketch_os(SKETCHER, std::ios::binary);
			if(sketch_os)
				feat.saveSketch(sketch_os);
			sketch_os.close();
		}
		else
			in.close();
	}
*/
	Timer timer; // start timing

/*
	std::cout << urls[0].size()  << std::endl;
	for(int i=0;i< urls[0].size();i++)
		std::cout << urls[0][i] << std::endl;
	extract(urls[0],os);
*/
	//bind to thread

	KeyDistributor key_dis(0);
	key_dis.setKey(atoi(argv[2]));

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
	return 1;
}
