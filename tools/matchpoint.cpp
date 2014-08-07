//match points
#include <stdio.h>
#include <math.h>
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

void drawResult(const std::vector<Pair> &match, const vector<Feature> &match_left, const vector<Feature> &match_right, const IplImage *img_left, const IplImage *img_right)
{
//	assert(match_left.size() == match_right.size());

	IplImage* big = stackImagesVertically(img_left, img_right, true);

	int x1,y1,x2,y2;

	float s1,s2;

	int idx1,idx2;

	Feature feat1,feat2;

	float s = 2 * M_PI;

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

		s1 = feat1.size * feat1.scale;
		s2 = feat2.size * feat2.scale;

		CvPoint dir1 = cvPoint(cvFloor(s1 * cos(feat1.dir * s)),cvFloor(s1 * sin(feat1.dir * s)));
		CvPoint dir2 = cvPoint(cvFloor(s2 * cos(feat2.dir * s)),cvFloor(s2 * sin(feat2.dir * s)));

		CvPoint left = cvPoint(cvRound(x1), cvRound(y1));
		CvPoint right = cvPoint(cvRound(x2), cvRound(img_left->height + y2));

		dir1 = cvPoint(left.x + dir1.x, left.y + dir1.y);
		dir2 = cvPoint(right.x + dir2.x, right.y + dir2.y);

		cvCircle(big, left, feat1.size * feat1.scale, RED);
		cvCircle(big, right, feat2.size * feat2.scale, RED);

		cvLine(big, left, dir1, YELLOW);
		cvLine(big, right, dir2, YELLOW);
		cvLine(big, left, right, GREEN);

	}

	cvSaveImage("match.png", big);
	cvReleaseImage(&big);
	printf("[OK] Matches shown in match.png\n\n");
}


int main(int argc, char ** argv)
{
	if(argc != 7)
	{
		cout << "Usage: matchpoint <src> <src_size> <dst> <dst_size> <hamming dist> <method>" << endl;
		return 0;
	}

	FeatureExtractor xtor1,xtor2;

	xtor1.setMaxImageSize(atoi(argv[2]));
	xtor2.setMaxImageSize(atoi(argv[4]));
	
//	vector<BitFeat> f1,f2;
	vector<Feature> f1,f2;

	int w1,h1,w2,h2;

	string s1(argv[1]);
	string s2(argv[3]);

	if(xtor1.loadImage(s1))
	{
		xtor1.getRawSift(f1);
		/*
		if(atoi(argv[6]))
			xtor1.getBitFeat(f1, SKETCH);
		else
			xtor1.getBitFeat(f1, LDAHASH);
		*/
	}
	else
	{
		cout << "Failed to load " << argv[1] << endl;
		return 0;
	}

	if(xtor2.loadImage(s2))
	{
		xtor2.getRawSift(f2);
		/*
                if(atoi(argv[6]))
                        xtor2.getBitFeat(f2, SKETCH);
                else
                        xtor2.getBitFeat(f2, LDAHASH);
		*/
	}
	else
	{
		cout << "Failed to load " << argv[2] << endl;
		return 0;
	}

	Hamming hamming;

	vector<Pair> result;

	int dist = atoi(argv[5]);
	float d;
	Point pt;

	float total = 0;

	float scale1,scale2;
	bool b_scale = true;

	for(int i=0;i<f1.size();i++)
		for(int j=0;j<f2.size();j++)
		{
			        if(b_scale)
                                {
                                        scale1 = f1[i].scale;
                                        scale2 = f2[j].scale;
                                }
                                else
                                {
                                        scale1 = 1.0;
                                        scale2 = 1.0;
                                }

			if((d = fabs(scale1 * f1[i].x - scale2 * f2[j].x) + fabs(scale1 * f1[i].y - scale2 * f2[j].y)) <= dist && fabs(f1[i].dir - f2[j].dir) < 0.05)
			{
				total += d;
/*
				if(b_scale)
				{
					scale1 = f1[i].scale;
					scale2 = f2[j].scale;
				}
				else
				{
					scale1 = 1.0;
					scale2 = 1.0;
				}
*/
				cout << "Match " << result.size() << endl;
				cout << "Points : " << i << "\t" << j << endl;
				cout << "Pos 1 : " << scale1*f1[i].x << "\t" << scale1*f1[i].y << endl;
				cout << "Pos 2 : " << scale2*f2[j].x << "\t" << scale2*f2[j].y << endl;
				cout << "Size : " << scale1*f1[i].size << "\t" << scale2*f2[j].size << endl;
				cout << "Dir : " << f1[i].dir << "\t" << f2[j].dir << endl;
				cout << "Point dist :" << d << endl;
				cout << "Desc dist :" << f1[i].dist(f2[j]) << endl;
				f1[i].printDesc();
				f2[j].printDesc();
				
				
				/*
				pt.key = result.size();
				pt.desc = f1[i].f;
				pt.printPoint();
				pt.desc = f2[j].f;
				pt.printPoint();
				cout << endl;
				*/

				result.push_back(Pair(i,j));
			}
		}

	float sim = ((float)(result.size())/f1.size() + (float)(result.size())/f2.size()) / 2;
	cout << "Src Image desc size: " << f1.size() << std::endl;
	cout << "Dst Image desc size: " << f2.size() << std::endl;
	cout << "Matched points size : " << result.size() << std::endl;
	cout << "Total dist : " << total << std::endl;
	cout << "Similarity : " << sim << std::endl;

	IplImage * top = cvLoadImage(argv[1], 3);
	IplImage * bottom = cvLoadImage(argv[3], 3);

	if(!top || !bottom)
	{
		cout << "Failed to load image." << endl;
		return 0;
	}

	drawResult(result, f1, f2, top, bottom);
	
	cvReleaseImage(&top);
	cvReleaseImage(&bottom);
	return 1;	
};

