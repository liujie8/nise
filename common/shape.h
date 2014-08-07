#ifndef _COMMON_SHAPE
#define _COMMON_SHAPE
//This file is used to extract shape features from image
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
	struct ShapeFeature {
		std::vector<float> desc;

		float dist(ShapeFeature &f)
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


	class ShapeExtractor
	{	
		IplImage *img;
		float scale;
		int max_image_size;

		enum{HSV, RGB}TYPE;

		public:	
	
		ShapeExtractor()
		{
			img = NULL;
			scale = 1.0;
			max_image_size = MAX_IMAGE_SIZE;			
		};

		~ShapeExtractor()
		{
			releaseImage();
		};

		int extractShape(const IplImage * src, double * featureVector, int type)
		{
			
			switch(type)
			{
				case HSV :break;
				case RGB : break;
				default: break;
			};
		};

	};

}
#endif
