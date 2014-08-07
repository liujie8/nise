#ifndef _COMMON_COLOR
#define _COMMON_COLOR
//This file is used to extract color features from image
#include <math.h>
#include <iostream>
#include <vector>
#include <string>

//opencv
#include <opencv/cv.h>
#include <opencv/highgui.h>

namespace nise
{
	/// Position of a feature
	struct ColorFeature {
		std::vector<float> desc;

		float dist(Feature &f)
		{
			float d = 0;

			for(int i=0;i<desc.size();i++)
				d += fabs(desc[i] - f.desc[i]);

			return d;
		};

		int size()
		{
			return desc.size();
		};

		void printDesc()
		{
			for(int i=0;i<desc.size();i++)
				std::cout << desc[i] << " ";
			std::cout << std::endl;
		};
	};


	class ColorExtractor
	{	
		IplImage *img;
		float sddcale;
		int max_image_size;

		enum{HSV, RGB}TYPE;

		public:	
	
		ColorExtractor()
		{
			img = NULL;
			scale = 1.0;
			max_image_size = MAX_IMAGE_SIZE;			
		};

		~ColorExtractor()
		{
			releaseImage();
		};

		int extractColor(const IplImage * src, double * featureVector, int type)
		{
			
			switch(type)
			{
				case HSV :break;
				case RGB : break;
				default: break;
			};
			
		};

		int get_HS_Hist(const IplImage * src, double * featureVector)
		{

			//      IplImage * src= cvLoadImage("lena.jpg");

			if(NULL == src || NULL == featureVector ||src->nChannels!=3)
				return 0;

			IplImage* hsv = cvCreateImage( cvGetSize(src), src->depth, 3 );
			IplImage* h_plane = cvCreateImage( cvGetSize(src), src->depth, 1 );
			IplImage* s_plane = cvCreateImage( cvGetSize(src), src->depth, 1 );
			IplImage* v_plane = cvCreateImage( cvGetSize(src), src->depth, 1 );
			IplImage* planes[] = { h_plane, s_plane };

			/** H ·ÖÁ¿»®·ÖÎª16¸öµÈ¼¶£¬S·ÖÁ¿»®·ÖÎª8¸öµÈ¼¶ */
			int h_bins = 6, s_bins = 6;
			int hist_size[] = {h_bins, s_bins};

			/** H ·ÖÁ¿µÄ±ä»¯·¶Î§ */
			float h_ranges[] = { 0, 180 };

			/** S ·ÖÁ¿µÄ±ä»¯·¶Î§*/
			float s_ranges[] = { 0, 255 };
			float* ranges[] = { h_ranges, s_ranges };

			/** ÊäÈëÍ¼Ïñ×ª»»µ½HSVÑÕÉ«¿Õ¼ä */
			cvCvtColor( src, hsv, CV_BGR2HSV );
			cvCvtPixToPlane( hsv, h_plane, s_plane, v_plane, 0 );

			/** ´´½¨Ö±·½Í¼£¬¶þÎ¬, Ã¿¸öÎ¬¶ÈÉÏ¾ù·Ö */
			CvHistogram * hist = cvCreateHist( 2, hist_size, CV_HIST_ARRAY, ranges, 1 );
			/** ¸ù¾ÝH,SÁ½¸öÆ½ÃæÊý¾ÝÍ³¼ÆÖ±·½Í¼ */
			cvCalcHist( planes, hist, 0, 0 );
			cvNormalizeHist(hist,1);

			for(int i=0;i<h_bins;i++)
				for(int j=0;j<s_bins;j++)
					featureVector[i*s_bins+j]=cvQueryHistValue_2D(hist,i,j);

			cvReleaseImage(&hsv);
			cvReleaseImage(&h_plane);
			cvReleaseImage(&s_plane);
			cvReleaseImage(&v_plane);
			cvReleaseHist(&hist);

			return 1;
		};

	};

}
#endif
