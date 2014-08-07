//match iamge
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "../extract/feature_extractor.h"
#include "../common/record.h"
#include "../common/hamming.h"


// OpenCV colors
// OpenCV colors
#define RED       CV_RGB(255,0,0)
#define GREEN     CV_RGB(0,255,0)
#define BLUE      CV_RGB(0,0,255)
#define BLACK     CV_RGB(0,0,0)
#define WHITE     CV_RGB(255,255,255)
#define GRAY      CV_RGB(190,190,190)
#define DKGRAY    CV_RGB(84,84,84)
#define LTGRAY    CV_RGB(211,211,211)
#define SILVER    CV_RGB(192,192,192)
#define LTBLUE    CV_RGB(135,206,235)
#define DKBLUE    CV_RGB(0,0,128)
#define LTRED     CV_RGB(255,69,0)
#define DKRED     CV_RGB(139,0,0)
#define LTGREEN   CV_RGB(0,255,127)
#define DKGREEN   CV_RGB(0,100,0)
#define ORANGE    CV_RGB(255,127,0)
#define YELLOW    CV_RGB(255,255,0)
#define MAGENTA   CV_RGB(139,0,139)
#define VIOLET    CV_RGB(79,47,79)

using namespace std;
using namespace nise;

struct Pair
{
	int left;
	int right;

	Pair(int a=0, int b=0)
	{
		left = a;
		right = b;
	};
};

/** Place top image on top of bottom image and return result
 **/
IplImage* stackImagesVertically(const IplImage *top, const IplImage *bottom, const bool big_in_color)
{
	if(top->depth != bottom->depth)return NULL;

	const int max_w = top->width > bottom->width ? top->width:bottom->width;

	IplImage *big;
	if (big_in_color)
	{
		big = cvCreateImage(cvSize(max_w, top->height + bottom->height), top->depth, 3);
		cvZero(big);
		IplImage *top_c = cvCreateImage(cvSize(top->width,top->height), top->depth, 3);
		IplImage *bottom_c = cvCreateImage(cvSize(bottom->width,bottom->height), bottom->depth, 3);
		cvConvertImage(top, top_c, CV_GRAY2RGB);
		cvConvertImage(bottom, bottom_c, CV_GRAY2RGB);
		char *p_big = big->imageData;
		for (int i=0; i<top_c->height; ++i, p_big+=big->widthStep)
			memcpy(p_big, top_c->imageData + i*top_c->widthStep,
					top_c->widthStep);
		for (int i=0; i<bottom_c->height; ++i, p_big+=big->widthStep)
			memcpy(p_big, bottom_c->imageData + i*bottom_c->widthStep,
					bottom_c->widthStep);
		cvReleaseImage(&top_c);
		cvReleaseImage(&bottom_c);
	}
	else
	{
		big = cvCreateImage(cvSize(max_w,top->height+bottom->height), top->depth, 1);
		cvZero(big);
		char *p_big = big->imageData;
		for (int i=0; i<top->height; ++i, p_big+=big->widthStep)
			memcpy(p_big, top->imageData + i*top->widthStep,
					top->widthStep);
		for (int i=0; i<bottom->height; ++i, p_big+=big->widthStep)
			memcpy(p_big, bottom->imageData + i*bottom->widthStep,
					bottom->widthStep);
	}

	return big;
}

void drawResult(const std::vector<Pair> &match, const vector<BitFeat> &match_left, const vector<BitFeat> &match_right, const IplImage *img_left, const IplImage *img_right)
{
//	assert(match_left.size() == match_right.size());

	IplImage* big = stackImagesVertically(img_left, img_right, true);

	int x1,y1,x2,y2;

	int idx1,idx2;

	BitFeat feat1,feat2;

	for(int n=0;n<match_left.size();n++)
	{
		feat1 = match_left[n];

		x1 = feat1.x * feat1.scale;
		y1 = feat1.y * feat1.scale;

		CvPoint left = cvPoint(cvRound(x1), cvRound(y1));

		cvCircle(big, left, feat1.size * 3, RED);
	}

	for(int m=0;m<match_right.size();m++)
	{
		feat2 = match_right[m];

		x2 = feat2.x * feat2.scale;
		y2 = feat2.y * feat2.scale;

		CvPoint right = cvPoint(cvRound(x2), cvRound(img_left->height + y2));
		cvCircle(big, right, feat2.size * 3, RED);
	}

	for (int i=0; i< match.size(); ++i) 
	{
		idx1 = match[i].left;
		idx2 = match[i].right;

		feat1 = match_left[idx1];
		feat2 = match_right[idx2];

		x1 = feat1.x * feat1.scale;
		y1 = feat1.y * feat1.scale;
		x2 = feat2.x * feat2.scale;
		y2 = feat2.y * feat2.scale;

		CvPoint left = cvPoint(cvRound(x1), cvRound(y1));
		CvPoint right = cvPoint(cvRound(x2), cvRound(img_left->height + y2));
		//		cvCircle(big, left, feat1.size * 3, RED);
		//		cvCircle(big, right, feat2.size * 3, RED);
		cvLine(big, left, right, GREEN);
	}

	cvSaveImage("matches.png", big);
	cvReleaseImage(&big);
	printf("[OK] Matches shown in matches.png\n\n");
}


int main(int argc, char ** argv)
{
	if(argc != 3)
	{
		cout << "Usage: matchimage <src1> <src2>" << endl;
		return 0;
	}

	FeatureExtractor xtor1,xtor2;
	
	vector<BitFeat> f1,f2;

	int w1,h1,w2,h2;

	string s1(argv[1]);
	string s2(argv[2]);

	if(xtor1.loadImage(s1))
		xtor1.getBitFeat(f1);
	else
	{
		cout << "Failed to load " << argv[1] << endl;
		return 0;
	}

	if(xtor2.loadImage(s2))
		xtor2.getBitFeat(f2);
	else
	{
		cout << "Failed to load " << argv[2] << endl;
		return 0;
	}

	Hamming hamming;

	vector<Pair> result;

	for(int i=0;i<f1.size();i++)
	for(int j=0;j<f2.size();j++)
	{
		if(hamming(f1[i].f.sketch,f2[j].f.sketch) <=3)
			result.push_back(Pair(i,j));		
	}
/*
	w1 = xtor1.getImage()->width;
	h1 = xtor1.getImage()->height;
	w2 = xtor2.getImage()->width;
	h2 = xtor2.getImage()->height;

	int maxwidth = w1>w2?w1:w2;
	int maxheight = h1+h2;
*/

	IplImage * top = cvLoadImage(argv[1], 3);
	IplImage * bottom = cvLoadImage(argv[2], 3);

	drawResult(result, f1, f2, top, bottom);
	
	cvReleaseImage(&top);
	cvReleaseImage(&bottom);
	return 1;	
};

