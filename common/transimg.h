#ifndef _IMAGE_TRANS
#define _IMAGE_TRANS

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <fstream>
#include <iostream>
#include <string>

#include "freeimage.h"

class ImageTrans
{
	IplImage * img;

public:
	ImageTrans()
	{
		img = NULL;
	};
	
	~ImageTrans()
	{
		if(img)cvReleaseImage(&img);
	};

	IplImage * getImg()
	{
		return img;
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
		if(img)
			cvReleaseImage(&img);
		img = NULL;
		return 1;
	};
	//resize image  
	IplImage * resizeImage(float ratio = 1)
	{
		if(!img)return NULL;
		
		if(ratio < 0)return NULL;
		
		CvSize size;
		size.width = img->width * ratio;
		size.height = img->height * ratio;

		IplImage *imgtrans = cvCreateImage(size, img->depth, img->nChannels);
	
		if(imgtrans)
			cvResize(img, imgtrans);
		return imgtrans;
	};


	IplImage* rotateImage(int angle, bool clockwise = true)  
	{  
		IplImage * src = img;
		if(!img)return NULL;

		angle = abs(angle) % 180;  
//		if (angle > 90)  
//		{  
//			angle = 90 - (angle % 90);  
//		}  
		IplImage* dst = NULL;  
		int width =  
			(double)(src->height * sin(angle * CV_PI / 180.0)) +  
			(double)(src->width * cos(angle * CV_PI / 180.0 )) + 1;  
		int height =  
			(double)(src->height * cos(angle * CV_PI / 180.0)) +  
			(double)(src->width * sin(angle * CV_PI / 180.0 )) + 1;  
		int tempLength = sqrt((double)src->width * src->width + src->height * src->height) + 10;  
		int tempX = (tempLength + 1) / 2 - src->width / 2;  
		int tempY = (tempLength + 1) / 2 - src->height / 2;  
		int flag = -1;  

		dst = cvCreateImage(cvSize(width, height), src->depth, src->nChannels);  
		cvZero(dst);  
		IplImage* temp = cvCreateImage(cvSize(tempLength, tempLength), src->depth, src->nChannels);  
		cvZero(temp);  

		cvSetImageROI(temp, cvRect(tempX, tempY, src->width, src->height));  
		cvCopy(src, temp, NULL);  
		cvResetImageROI(temp);  

		if (clockwise)  
			flag = 1;  

		float m[6];  
		int w = temp->width;  
		int h = temp->height;  
		m[0] = (float) cos(flag * angle * CV_PI / 180.);  
		m[1] = (float) sin(flag * angle * CV_PI / 180.);  
		m[3] = -m[1];  
		m[4] = m[0];  
		// ½«ÐתÖÐÒÖͼÏÖ¼ä
		m[2] = w * 0.5f;  
		m[5] = h * 0.5f;  
		//  
		CvMat M = cvMat(2, 3, CV_32F, m);  
		cvGetQuadrangleSubPix(temp, dst, &M);  
		cvReleaseImage(&temp);  
		return dst;  
	}; 

};

#endif

