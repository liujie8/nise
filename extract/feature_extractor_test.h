#ifndef _DESC_WRAPPER
#define _DESC_WRAPPER
//This file is used to extract features from image

#include <iostream>
#include <vector>
#include <string>
#include <bitset>
#include <boost/foreach.hpp>
#include <boost/assert.hpp>
#include <boost/thread/mutex.hpp>

//opencv
#include <opencv/cv.h>
#include <opencv/highgui.h>

//freeimage
#include "../common/FreeImage.h"

#include "../common/record.h"
#include "../common/rawrecord.h"
#include "../common/config.h"
#include "../common/io.h"
#include "../common/eval.h"

//sift
extern "C" {
#include "generic.h"
#include "sift.h"
}
//ldahash
#include "ldahash.h"

//lskkit
#include "lshkit.h"

namespace nise
{
	/// Position of a feature
	struct Feature {
		float x, y, size, dir;  // dir in [0,1]
		float scale;    // 
		std::vector<float> desc;
	};

//	static const int SIFT_DIM = 128;

	static const unsigned SAMPLE_RANDOM = 0;
	static const unsigned SAMPLE_SIZE = 1;

	static inline bool sift_by_size (const Feature &f1, const Feature &f2) {
		return f1.size > f2.size;
	};
	
	//descripor binarzation method	
	static const unsigned LDAHASH = 0;
	static const unsigned SKETCH = 1;

	//lsh
	typedef lshkit::Sketch<lshkit::DeltaLSB<lshkit::GaussianLsh>, Chunk> LshSketch;

	class FeatureExtractor
	{	
		IplImage *img;
		float scale;
		int rawfeat; //sift num

		bool b_resize;
		bool b_entropy;
		bool b_logscale;
		bool b_blacklist;
		
		//parameters for feature extraction
		static const unsigned DIM = SIFT_DIM;
		int O, S, omin;
		double edge_thresh;
		double peak_thresh;
		double magnif;
		double e_th;
		bool do_angle;
		unsigned R;

		float log_base;

		unsigned max_feat;
		unsigned sample_method;

		std::vector<std::vector<float> > black_list;
		float threshold;

		//lsh
		LshSketch sketch;

		public:		
		FeatureExtractor()
		{
			img = NULL;
			scale = 1.0;
			
			b_resize = true;
			b_entropy = true;
			b_logscale = true;
			b_blacklist = true;

			//default parameter settings for sift
			O = -1;
			S = 3;
			omin = 0;
			edge_thresh = -1;
			peak_thresh = 3;  //default 3
			magnif = -1;
			e_th = MIN_ENTROPY;
			do_angle = true;
			R = 256;			
			
			log_base = LOG_BASE;
			max_feat = MAX_FEATURES;
			sample_method = SAMPLE_SIZE;

			//lsh
			lshkit::DefaultRng rng;
			LshSketch::Parameter pm;
			pm.W = 8;
			pm.dim = 128; // Sift::dim();
			sketch.reset( DATA_CHUNK, pm, rng);
		};

		~FeatureExtractor()
		{
			releaseImage();
		};

		IplImage * getImg()
		{
			return img;
		};
		
		int getRawSiftNum()
		{
			return rawfeat;
		};

		//support for gif image
		bool isGif(const char* filePath)
		{
			std::ifstream inFile;
			inFile.open(filePath,std::ios::in);
			unsigned char gif_type[4] = {0x47,0x49,0x46,' '};
			unsigned char file_head[4];
			file_head[3] = ' ';
			for (int i=0;i<3;++i)
			{
				inFile>>file_head[i];
				if(file_head[i]!=gif_type[i])
				{
					inFile.close();
					return false;
				}
			}
			inFile.close();
			return true;
		}

		IplImage*  gif2ipl(const char* filename, bool bgray = true)  
		{  
			FreeImage_Initialise();         //load the FreeImage function lib  
			FREE_IMAGE_FORMAT fif = FIF_GIF;  
			FIBITMAP* fiBmp = FreeImage_Load(fif,filename,GIF_DEFAULT);  

			//  FIBITMAPINFO fiBmpInfo = getfiBmpInfo(fiBmp);  

			int width = FreeImage_GetWidth(fiBmp);;  
			int height = FreeImage_GetHeight(fiBmp);  
			//  int depth = fiBmpInfo.depth;  
			//  int widthStep = (width*depth+31)/32*4;  
			//  size_t imgDataSize = widthStep*height*3;  

			RGBQUAD* ptrPalette = FreeImage_GetPalette(fiBmp);  

			BYTE intens;  
			BYTE* pIntensity = &intens;  

			IplImage* iplImg = cvCreateImage(cvSize(width,height),IPL_DEPTH_8U,3);  

			iplImg->origin = 1;//should set to 1-top-left structure(Windows bitmap style)  

			for (int i=0;i<height;i++)  
			{  
				char* ptrImgDataPerLine = iplImg->imageData + i*iplImg->widthStep;  

				for(int j=0;j<width;j++)  
				{  
					//get the pixel index   
					FreeImage_GetPixelIndex(fiBmp,j,i,pIntensity);    

					ptrImgDataPerLine[3*j] = ptrPalette[intens].rgbBlue;  
					ptrImgDataPerLine[3*j+1] = ptrPalette[intens].rgbGreen;  
					ptrImgDataPerLine[3*j+2] = ptrPalette[intens].rgbRed;  
				}  
			}  

			FreeImage_Unload(fiBmp);  
			FreeImage_DeInitialise();                 
			
			if(bgray)
			{
				IplImage * old = iplImg;
				iplImg = cvCreateImage(cvSize(width,height),IPL_DEPTH_8U,1);
				cvCvtColor(old, iplImg, CV_BGR2GRAY);
				cvReleaseImage(&old);
			}

			return iplImg;  
		}  
		//load image
		int loadImage(std::string &src)
		{
			releaseImage();
	
			img = cvLoadImage(src.c_str(),CV_LOAD_IMAGE_GRAYSCALE);	
	
			if(img == NULL && isGif(src.c_str()))
				img = gif2ipl(src.c_str());

			if(!img)return 0;
			else 
				return 1;
		};
		
		//release image
		int releaseImage()
		{
			if(!img)return 0;
			else
				cvReleaseImage(&img);
			img = NULL;
			return 1;
		};
		
		//resize image	
		int resizeImage(int max = MAX_IMAGE_SIZE)
		{
			if(!img)return 0;
			
			scale = 1.0;
			if(img->width <= max && img->height <= max)return 1;
			
			CvSize size;
			float ratio = 0;
			if (img->width < img->height) {
				ratio = img->height/img->width;
				if(ratio > 4 && ratio < 100)
				{
					size.width = 100;
					size.height = ratio * 100;
					scale = img->width/100;
				}
				else if(ratio >=100)
				{
					size.height = img->height;
					size.width = img->width;
					scale = 1.0;
				}
				else
				{
					scale = img->height / max;
					size.width = img->width * max / img->height;
					size.height = max;
				}
			}
			else {
				ratio = img->width/img->height;
				if(ratio > 4 && ratio < 100)
				{
					size.height = 100;
					size.width = ratio * 100;
					scale = img->height/100;
				}
				else if(ratio >=100)
				{
					size.height = img->height;
					size.width = img->width;
					scale = 1.0;
				}
				else
				{
					scale = img->width / max;
					size.width = max;
					size.height = img->height * max / img->width;
				}
			}
			IplImage *old = img;
			img = cvCreateImage(size, old->depth, old->nChannels);
			cvResize(old, img);
			cvReleaseImage(&old);
		};
		
		void loadSketch(std::ifstream &is)
		{
			sketch.load(is);
		};

		void saveSketch(std::ofstream &os)
		{
			sketch.save(os);
		};

		unsigned long long getFingerPrint()
		{
			//get a 64 bits fingerprint of the image
			
			if(!img)return 0;

			CvSize size;
			size.height = 12;
			size.width = 8;
			
			if(img->nChannels != 1)return 0;

			IplImage * fingerImage = cvCreateImage(size, img->depth, img->nChannels);
			if(fingerImage == NULL)return 0;

			cvResize(img, fingerImage);
			
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
				for(int j=0;j<8;j++)
				{
					s = cvGet2D(fingerImage, i,j);
					if( s.val[0] > mean)
						fingerprint = fingerprint ^ mask[i*8+j];
				}

			cvReleaseImage(&fingerImage);
			
			return fingerprint;
		};

		//calc the entropy of a descriptor
		float entropy (const std::vector<float> &desc) 
		{  
			std::vector<unsigned> count(256); 
			fill(count.begin(), count.end(), 0);     
			BOOST_FOREACH(float v, desc) 
			{     
				unsigned c = round(v * 255);   
				if (c < 256) count[c]++;      
			}   
			float e = 0;    
			BOOST_FOREACH(unsigned c, count) 
			{        
				if (c > 0) {           
					float pr = (float)c / (float)desc.size();           
					e += - pr * log2(pr);       
				}       
			}        return e;  
		};

		//set to blacklist
		void setBlackList (const std::string &path, float t) 
		{       
			threshold = t;    
			std::vector<float> tmp(SIFT_DIM);      
			std::ifstream is(path.c_str(), std::ios::binary);  
			while (is.read((char *)&tmp[0], sizeof(float) * SIFT_DIM)) 
			{ 
				black_list.push_back(tmp);   
			}  
		};
/*	
		bool checkBlackList (const std::vector<float> &desc) {
			lshkit::metric::l2<float> l2(Sift::dim());
			BOOST_FOREACH(const std::vector<float> &b, black_list) {
				// if (l2(&desc[0], &b[0]) < threshold) return false;
				float d = l2(&desc[0], &b[0]);
				if (d < threshold) return false;
			}
			return true;
		};
*/
		void sampleFeature (std::vector<Feature> *sift, unsigned count, unsigned method) {
			if (sift->size() < count) return;
			if (method == SAMPLE_RANDOM) {
				std::random_shuffle(sift->begin(), sift->end());
			}
			else if (method == SAMPLE_SIZE) {
				std::sort(sift->begin(), sift->end(), sift_by_size);
			}
			else {
				BOOST_VERIFY(0);
			}
			sift->resize(count);
		}

		void logscale (std::vector<Feature> *v, float l) {
			float s = -log(l);
			BOOST_FOREACH(Feature &f, *v) {
				BOOST_FOREACH(float &d, f.desc) {
					if (d < l) d = l;
					d = (log(d) + s) / s;
					if (d > 1.0) d = 1.0;
				}
			}
		}

 		int extractSiftDesc(std::vector<Feature> *list)
		{
			if(!img)return 0;

		//	if(b_resize)
			//	resizeImage();

			if(img->width < MIN_IMAGE_SIZE || img->height < MIN_IMAGE_SIZE)return 0;
	//		std::cout <<"point 1"<< std::endl;

			Timer timer;
			float *fdata = new float[img->width * img->height];
			BOOST_VERIFY(fdata);
			{
				float *d = fdata;
				unsigned char *l = (unsigned char *)img->imageData;
				for (int h = 0; h < img->height; ++h) {
					for (int w = 0; w < img->width; ++w) {
						*d = float(l[w]);
						d++;
					}
					l += img->widthStep;
				}
			}

			VlSiftFilt *filt = vl_sift_new (img->width, img->height, O, S, omin) ;

//			std::cout << "timer 1:" << timer.elapsed() << std::endl;
//			timer.restart();

			BOOST_VERIFY(filt);
			if (edge_thresh >= 0) vl_sift_set_edge_thresh (filt, edge_thresh) ;
			if (peak_thresh >= 0) vl_sift_set_peak_thresh (filt, peak_thresh) ;
			if (magnif      >= 0) vl_sift_set_magnif      (filt, magnif) ;

			magnif = vl_sift_get_magnif(filt);

			list->clear();
	//		std::cout << "timer 1.5:" << timer.elapsed() << std::endl;
	//		timer.restart();
	//		std::cout << "point 2" << std::endl;
			int err = vl_sift_process_first_octave (filt, fdata);

//			std::cout << "timer 2:" << timer.elapsed() << std::endl;
//			timer.restart();
			rawfeat = 0;
	//		std::cout << "point 2.5" << std::endl;
			while (err == 0) {
	//			std::cout << "point 3" << std::endl;
				vl_sift_detect (filt) ;

//				std::cout << "timer 3:" << timer.elapsed() << std::endl;
//				timer.restart();

				VlSiftKeypoint const *keys  = vl_sift_get_keypoints(filt) ;
				int nkeys = vl_sift_get_nkeypoints(filt) ;
	//			std::cout << "point 3" << std::endl;
				for (int i = 0; i < nkeys ; ++i) {
					VlSiftKeypoint const *key = keys + i;
					double                angles [4] ;

					int nangles = 0;
					if (do_angle) {
						nangles = vl_sift_calc_keypoint_orientations(filt, angles, key) ;
					}
					else {
						nangles = 1;
						angles[0] = 0;
					}

					for (int q = 0 ; q < nangles ; ++q) {

						list->push_back(Feature());
						Feature &f = list->back();
						f.desc.resize(DIM);
						/* compute descriptor (if necessary) */
						vl_sift_calc_keypoint_descriptor(filt, &f.desc[0], key, angles[q]) ;

						BOOST_FOREACH(float &v, f.desc) {
							/*
							   v = round(v * SIFT_RANGE);
							   if (v > SIFT_RANGE) v = SIFT_RANGE;
							 */
							v *= 2;
							if (v > 1.0) v = 1.0;
						}

						f.scale = scale;
						f.x = key->x;
						f.y = key->y;
						f.dir = angles[q] / M_PI / 2;
						f.size = key->sigma * magnif;

						rawfeat++;
						if ((entropy(f.desc) < e_th) /*|| !checkBlackList(f.desc)*/) {
							list->pop_back();
						}

					}
				}
//				std::cout << "timer 4:" << timer.elapsed() << std::endl;
//				timer.restart();
	//			std::cout << "point 4" << std::endl;
				err = vl_sift_process_next_octave(filt) ;
			}
//			std::cout << "rawfeat: " << rawfeat << std::endl;
			vl_sift_delete (filt) ;
			delete [] fdata;

			return list->size();
		};
/*	
		//detect feature point
		int detectKeypoint(std::vector<Keypoint> &keypoint)
		{
			if(!img)return 0;
			
			if(img->width < MIN_IMAGE_SIZE || img->height < MIN_IMAGE_SIZE)return 0;

			if(b_resize)
				resizeImage();
			
			

		};
 */

		int getSiftDesc(std::vector<SiftFeature> &desc)
		{
			if(!img)return 0;
			
			std::vector<Feature> sift;

			//extract sift desc
			extractSiftDesc(&sift);
			//logscale
			if(b_logscale && log_base > 0)
				logscale(&sift,log_base);
			//sample desc   
			if(max_feat > 0 )
				sampleFeature(&sift, max_feat, sample_method);
			desc.resize(sift.size());
			
			if(sift.size()==0)return 0;

			for(int i=0;i< sift.size();i++)
				for(int j=0;j< SIFT_DIM; j++)
					desc[i].sketch[j] = sift[i].desc[j];
			
			return sift.size();
			
		};

		//get bit string descripors
		int getBitDesc(std::vector<BitFeature> &desc, int bin_method = LDAHASH, bool sim_hash = false)
		{
			if(!img)return 0;

			std::vector<Feature> sift;

			//extract sift desc
	//		std::cout << "Start extract sift." << std::endl;
  			extractSiftDesc(&sift);
	//		std::cout << "Finish extract sift." << std::endl;
			//logscale
  			if(b_logscale && log_base > 0)
	    			logscale(&sift,log_base);
	//		std::cout << "logscale finished." << std::endl;
			//sample desc	
			if(max_feat > 0 )
				sampleFeature(&sift, max_feat, sample_method);
	//		std::cout << "sampleFeature finished." << std::endl;
			desc.resize(sift.size());
			
			if(sift.size()==0)return 0;

			//binarize desc
			switch(bin_method)
			{
				//ldahash
				case LDAHASH:
					{
						Ldahash lda;
						for(int i = 0;i < sift.size();i++){
							lda.run_sifthash(&sift[i].desc[0]);
							unsigned long long * res = (unsigned long long *)desc[i].sketch;
							res[0] = lda.get_b1();
							res[1] = lda.get_b2();
						}
						
						break;
					}
				//sketch
				case SKETCH:
					{
						for(int i=0;i < sift.size();i++)
							sketch.apply(&sift[i].desc[0], desc[i].sketch);
						break;
					}
				default: break;
			}
/*	
			//simhash to further compress desc
			if(sim_hash)
			{
				std::vector<float> sum(64 * BLOCK_SIZE, 0);
			//	sum.resize(64 * BLOCK_SIZE);
				
				unsigned long long bitword = 1;	
				//sum of weight of sift feature
				for(int i=0;i< desc.size();i++)
					for(int j=0;j< BLOCK_SIZE;j++)
					{	
						for(int k=0;k < 64;k++)
							if(desc[i].sketch[j] & (bitword << k))
								sum[j*64 + k] += sift[i].size;
							else
								sum[j*64 + k] -= sift[i].size;
					}	
				//binarize the sum
				BitFeature bitfeat;
				for(int i=0;i<BLOCK_SIZE;i++)
					bitfeat.sketch[i]=0;

				for(int i=0;i< sum.size();i++)
					if(sum[i] > 0)
						bitfeat.sketch[i/64] |= (bitword << (i%64));
				
				desc.clear();
				desc.push_back(bitfeat);
			}
*/
			return desc.size();

		};	

	};

}
#endif
