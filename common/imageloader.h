#ifndef _COMMON_IMAGELOADER
#define _COMMON_IMAGELOADER

#include <iostream>
#include <vector>
#include <string>

//image loader
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "FreeImage.h"

namespace nise
{
	class ImageLoader
	{
		public:
			ImageLoader()
			{
				images.clear();
			};

			~ImageLoader()
			{
				releaseImage();
			};

			IplImage * getImg(int idx)
			{
				if(idx < images.size() && idx >= 0)
					return images[idx];
				else
					return NULL;
			};

			void release()
			{
				releaseImage();
			};

			int size()
			{
				return images.size();
			};

			//load image
			int loadImage(const char * src, bool bGray = true)
			{
				releaseImage();
		//		loadImageByOpenCV(src, bGray);
				loadImageByFreeImage(src, bGray);

			//	if(!loadImageByOpenCV(src, bGray))loadImageByFreeImage(src, bGray);

				return images.size();
			};

		private:
			void releaseImage()
			{
				for(int i=0; i< images.size(); i++)
				{
					if(images[i])cvReleaseImage(&images[i]);
				}

				images.clear();
			};

			int loadImageByOpenCV(const char * filename, bool bGray = true)
			{
				IplImage * image = NULL;

				image = cvLoadImage(filename, bGray?CV_LOAD_IMAGE_GRAYSCALE:CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);

				if(image)images.push_back(image);

				return images.size();
			};

			int loadImageByFreeImage(const char * filename, bool bGray = true)
			{
				FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

				fif = FreeImage_GetFileType(filename, 0);

				if(FIF_UNKNOWN == fif || !FreeImage_FIFSupportsReading(fif))return 0;

				if (FIF_GIF == fif || FIF_TIFF == fif || FIF_ICO == fif)
				{
					FIMULTIBITMAP * mdib = FreeImage_OpenMultiBitmap(fif, filename, false, true);
					if(mdib) 
					{
						int pages = FreeImage_GetPageCount(mdib);
						if(pages > 10)pages = 10;
						for(int page = 0; page < pages; ++page) 
						{
							FIBITMAP * dib = FreeImage_LockPage(mdib, page);
							if(dib)
							{
								Fibitmap2IplImage(dib);
								FreeImage_UnlockPage(mdib, dib, false);
							}
						}
						FreeImage_CloseMultiBitmap(mdib);
					}
			
				}
				else
				{
					FIBITMAP * dib = FreeImage_Load(fif, filename);
					if(dib)
					{
						Fibitmap2IplImage(dib);
						FreeImage_Unload(dib);
					}
				}

				//convert to gray image
				if(bGray && !Rgb2Grayscale())
				{
			//		return 0;
				}

				return images.size();
			};

			bool Fibitmap2IplImage(FIBITMAP * bitmap)
			{
				if (NULL == bitmap)
					return false;

				bool bRet;

				RGBQUAD * ptrPalette = FreeImage_GetPalette(bitmap);

				if (!ptrPalette)
				{
					bRet = Fibitmap2IplImageNoPalette(bitmap);
				}
				else
				{
					bRet = Fibitmap2IplImageWithPalette(bitmap, ptrPalette);
				}

				return bRet;
			};

			bool Rgb2Grayscale()
			{
				int size = images.size();

				for (int i = 0; i < size; ++i)
				{
					IplImage * image = images[i];

					if (NULL == image)
						continue;

					if (image->nChannels > 2)
					{
						if (image->depth != IPL_DEPTH_8U)
						{
							IplImage * tmp = cvCreateImage(cvSize(image->width, image->height), IPL_DEPTH_8U, image->nChannels);
							if (NULL == tmp)
							{
								return false;
							}

							cvConvertScale(image, tmp);
							cvReleaseImage(&image);

							images[i] = tmp;
							image = tmp;
						}

						IplImage * gray_image = cvCreateImage(cvSize(image->width, image->height), IPL_DEPTH_8U, 1);

						if (NULL == gray_image)
						{
							return false;
						}

						if (3 == image->nChannels || 4 == image->nChannels)
							cvCvtColor(image, gray_image, CV_BGR2GRAY);

						images[i] = gray_image;
						cvReleaseImage(&image);
					}

				}

				return true;
			};

			bool Fibitmap2IplImageWithPalette(FIBITMAP * bitmap, RGBQUAD * ptrPalette)
			{
				if ((NULL == bitmap) || (NULL == ptrPalette) && ((ptrPalette = FreeImage_GetPalette(bitmap)) == NULL)) return false;

				int nClrUsed = FreeImage_GetColorsUsed(bitmap);
				if (nClrUsed <= 0) return false;

				unsigned int width  = FreeImage_GetWidth(bitmap);
				unsigned int height = FreeImage_GetHeight(bitmap);

				if(width <= 0 || height <= 0) return false;

				IplImage * pImg = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);

				if (NULL == pImg) return false;

				for (unsigned int i = 0; i < height; ++i)
				{
					BYTE * ptrDataLine = (BYTE *)pImg->imageData + (height - i - 1) * pImg->widthStep;
					BYTE clrIndex;

					for(unsigned int j = 0; j < width; ++j)
					{
						if (!FreeImage_GetPixelIndex(bitmap, j , i, &clrIndex) || (int)clrIndex >= nClrUsed)
						{
							cvReleaseImage(&pImg);
							return false;
						}

						ptrDataLine[3 * j + 0] = ptrPalette[clrIndex].rgbBlue;
						ptrDataLine[3 * j + 1] = ptrPalette[clrIndex].rgbGreen;
						ptrDataLine[3 * j + 2] = ptrPalette[clrIndex].rgbRed;
					}
				}

				images.push_back(pImg);
				return true;
			};

			bool Fibitmap2IplImageNoPalette(FIBITMAP * bitmap)
			{	
				if (NULL == bitmap) return false;

				unsigned int imageWidth = FreeImage_GetWidth(bitmap);
				unsigned int imageHeight = FreeImage_GetHeight(bitmap);

				if(imageWidth <= 0 || imageHeight <= 0) return false;

				int channels;
				int depth = FreeImage_GetBPP(bitmap);

				FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(bitmap);

				bool bRet;

				switch (image_type)
				{
					case FIT_BITMAP://standard image : 1-, 4-, 8-, 16-, 24-, 32-bit
						{
							FIBITMAP * bitmap_temp = NULL;

							if (depth < 8) 
							{
								bitmap_temp = FreeImage_ConvertToGreyscale(bitmap);

								if (NULL == bitmap_temp)
								{
									bRet = false;
									break;
								}

								depth = 8;
								channels = 1;
							}
							else
							{
								if (depth % 8 != 0)
								{
									bRet = false;
									break;
								}

								channels = depth / 8;

								depth = 8;
							}

							if (channels > 4)
							{
								bRet = false;

								if(bitmap_temp)
									FreeImage_Unload(bitmap_temp);

								break;
							}

							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), depth, channels);

							if (NULL == pImg)
							{
								bRet = false;

								if(bitmap_temp) 
									FreeImage_Unload(bitmap_temp);
								break;
							}

							FIBITMAP * tmp_dib = bitmap_temp?bitmap_temp:bitmap;

							switch (channels)
							{
								case 1:
									{
										for (unsigned int i = 0; i < imageHeight; ++i)
										{
											BYTE * pDibLine = (BYTE *)FreeImage_GetScanLine(tmp_dib, i);
											BYTE * pIplLine = (BYTE *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
											for(unsigned int j = 0; j < imageWidth; ++j)
											{
												pIplLine[j] = pDibLine[j];
											}
										}

										break;
									}

								case 2:

									{
										return false;
									}

								case 3://执行过
									{
										for (unsigned int i = 0; i < imageHeight; ++i)
										{
											RGBTRIPLE * pDibLine = (RGBTRIPLE *)FreeImage_GetScanLine(tmp_dib, i);

											BYTE * pIplLine = (BYTE *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);

											for(unsigned int j = 0; j < imageWidth; ++j)
											{
												pIplLine[3 * j + 0] = pDibLine[j].rgbtBlue;
												pIplLine[3 * j + 1] = pDibLine[j].rgbtGreen;
												pIplLine[3 * j + 2] = pDibLine[j].rgbtRed;
											}
										}
										break;
									}

								case 4:
									{
										for (unsigned int i = 0; i < imageHeight; ++i)
										{
											RGBQUAD * pDibLine = (RGBQUAD *)FreeImage_GetScanLine(tmp_dib, i);
											BYTE * pIplLine = (BYTE *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);
											for(unsigned int j = 0; j < imageWidth; ++j)
											{
												pIplLine[4 * j + 0] = pDibLine[j].rgbBlue;
												pIplLine[4 * j + 1] = pDibLine[j].rgbGreen;
												pIplLine[4 * j + 2] = pDibLine[j].rgbRed;
												pIplLine[4 * j + 3] = pDibLine[j].rgbReserved;
											}
										}
										break;
									}

							}

							images.push_back(pImg); 
							bRet = true;
							if(bitmap_temp) 
								FreeImage_Unload(bitmap_temp);
							break;

						}

					case FIT_UINT16://array of unsigned short : unsigned 16-bit
						{
							if (depth != 16)
							{
								bRet = false;
								break;
							}

							channels = 1;

							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_16U, channels);

							if (NULL == pImg)
							{
								bRet = false;
								break;
							}

							for (unsigned int i = 0; i < imageHeight; ++i)
							{
								unsigned short * pDibLine = (unsigned short *)FreeImage_GetScanLine(bitmap, i);
								unsigned short * pIplLine = (unsigned short *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
								for(unsigned int j = 0; j < imageWidth; ++j)
								{
									pIplLine[j] = pDibLine[j];
								}
							}

							images.push_back(pImg); 

							bRet = true;
							break;
						}

					case FIT_INT16://array of short : signed 16-bit
						{
							if (depth != 16)
							{
								bRet = false;
								break;
							}

							channels = 1;

							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_16S, channels);
							if (NULL == pImg)
							{
								bRet = false;
								break;
							}

							for (unsigned int i = 0; i < imageHeight; ++i)
							{
								short * pDibLine = (short *)FreeImage_GetScanLine(bitmap, i);
								short * pIplLine = (short *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
								for(unsigned int j = 0; j < imageWidth; ++j)
								{
									pIplLine[j] = pDibLine[j];
								}
							}

							images.push_back(pImg); 

							bRet = true;
							break;
						}

					case FIT_UINT32://array of unsigned long : unsigned 32-bit
						{
							if (depth != 32)
							{
								bRet = false;
								break;
							}

							channels = 1;

							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_32F, channels);

							if (NULL == pImg)
							{
								bRet = false;
								break;
							}

							for (unsigned int i = 0; i < imageHeight; ++i)
							{
								DWORD * pDibLine = (DWORD *)FreeImage_GetScanLine(bitmap, i);
								float * pIplLine = (float *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
								for(unsigned int j = 0; j < imageWidth; ++j)
								{
									pIplLine[j] = pDibLine[j];
								}
							}

							images.push_back(pImg); 

							bRet = true;
							break;
						}

					case FIT_INT32://array of long : signed 32-bit

						{
							if (depth != 32)
							{
								bRet = false;
								break;
							}

							channels = 1;

							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_32S, channels);

							if (NULL == pImg)
							{
								bRet = false;
								break;
							}

							for (unsigned int i = 0; i < imageHeight; ++i)
							{
								LONG * pDibLine = (LONG *)FreeImage_GetScanLine(bitmap, i);
								LONG * pIplLine = (LONG *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
								for(unsigned int j = 0; j < imageWidth; ++j)
								{
									pIplLine[j] = pDibLine[j];
								}
							}

							images.push_back(pImg); 

							bRet = true;
							break;
						}

					case FIT_FLOAT://array of float : 32-bit IEEE floating point

						{
							if (depth != 32)
							{
								bRet = false;
								break;
							}

							channels = 1;

							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_32F, channels);

							if (NULL == pImg)
							{
								bRet = false;
								break;
							}

							for (unsigned int i = 0; i < imageHeight; ++i)
							{
								float * pDibLine = (float *)FreeImage_GetScanLine(bitmap, i);
								float * pIplLine = (float *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
								for(unsigned int j = 0; j < imageWidth; ++j)
								{
									pIplLine[j] = pDibLine[j];
								}
							}

							images.push_back(pImg); 

							bRet = true;
							break;

						}

					case FIT_DOUBLE://array of double : 64-bit IEEE floating point
						{
							if (depth != 64)
							{
								bRet = false;
								break;
							}

							channels = 1;
							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_64F, channels);
							if (NULL == pImg)
							{
								bRet = false;
								break;
							}

							for (unsigned int i = 0; i < imageHeight; ++i)
							{
								double * pDibLine = (double *)FreeImage_GetScanLine(bitmap, i);
								double * pIplLine = (double *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
								for(unsigned int j = 0; j < imageWidth; ++j)
								{
									pIplLine[j] = pDibLine[j];
								}
							}

							images.push_back(pImg); 

							bRet = true;
							break;

						}

					case FIT_COMPLEX://array of FICOMPLEX : 2 x 64-bit IEEE floating point

						{
							if (depth != 64 * 2)
							{
								bRet = false;
								break;
							}

							channels = 2;

							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_64F, channels);

							if (NULL == pImg)
							{
								bRet = false;
								break;
							}

							for (unsigned int i = 0; i < imageHeight; ++i)
							{
								FICOMPLEX * pDibLine = (FICOMPLEX *)FreeImage_GetScanLine(bitmap, i);
								double * pIplLine = (double *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
								for(unsigned int j = 0; j < imageWidth; ++j)
								{
									pIplLine[2 * j + 0] = pDibLine[j].r;
									pIplLine[2 * j + 1] = pDibLine[j].i;
								}
							}

							images.push_back(pImg); 

							bRet = true;
							break;

						}

					case FIT_RGB16://48-bit RGB image : 3 x 16-bit         //执行过

						{
							if (depth != 16 * 3)
							{
								bRet = false;
								break;
							}

							channels = 3;

							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_16U, channels);

							if (NULL == pImg)
							{
								bRet = false;
								break;
							}

							for (unsigned int i = 0; i < imageHeight; ++i)
							{
								FIRGB16 * pDibLine = (FIRGB16 *)FreeImage_GetScanLine(bitmap, i);
								unsigned short * pIplLine = (unsigned short *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
								for(unsigned int j = 0; j < imageWidth; ++j)
								{
									pIplLine[3 * j + 0] = pDibLine[j].blue;
									pIplLine[3 * j + 1] = pDibLine[j].green;
									pIplLine[3 * j + 2] = pDibLine[j].red;
								}
							}

							images.push_back(pImg); 

							bRet = true;
							break;

						}

					case FIT_RGBF://96-bit RGB float image : 3 x 32-bit IEEE floating point
						{
							if (depth != 32 * 3)
							{
								bRet = false;
								break;
							}

							channels = 3;

							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_32F, channels);

							if (NULL == pImg)
							{
								bRet = false;
								break;
							}

							for (unsigned int i = 0; i < imageHeight; ++i)
							{
								FIRGBF * pDibLine = (FIRGBF *)FreeImage_GetScanLine(bitmap, i);
								float * pIplLine = (float *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
								for(unsigned int j = 0; j < imageWidth; ++j)
								{
									pIplLine[3 * j + 0] = pDibLine[j].blue;
									pIplLine[3 * j + 1] = pDibLine[j].green;
									pIplLine[3 * j + 2] = pDibLine[j].red;
								}
							}

							images.push_back(pImg); 

							bRet = true;
							break;

						}

					case FIT_RGBA16://64-bit RGBA image : 4 x 16-bit

						{
							if (depth != 16 * 4)
							{
								bRet = false;
								break;
							}

							channels = 4;

							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_16U, channels);

							if (NULL == pImg)
							{
								bRet = false;
								break;
							}

							for (unsigned int i = 0; i < imageHeight; ++i)
							{
								FIRGBA16 * pDibLine = (FIRGBA16 *)FreeImage_GetScanLine(bitmap, i);
								unsigned short * pIplLine = (unsigned short *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
								for(unsigned int j = 0; j < imageWidth; ++j)
								{
									pIplLine[4 * j + 0] = pDibLine[j].blue;
									pIplLine[4 * j + 1] = pDibLine[j].green;
									pIplLine[4 * j + 2] = pDibLine[j].red;
									pIplLine[4 * j + 3] = pDibLine[j].alpha;
								}
							}

							images.push_back(pImg); 

							bRet = true;
							break;

						}

					case FIT_RGBAF://128-bit RGBA float image : 4 x 32-bit IEEE floating point

						{

							if (depth != 32 * 4)
							{
								bRet = false;
								break;
							}

							channels = 4;

							IplImage * pImg = cvCreateImage(cvSize(imageWidth, imageHeight), IPL_DEPTH_32F, channels);

							if (NULL == pImg)
							{
								bRet = false;
								break;
							}

							for (unsigned int i = 0; i < imageHeight; ++i)
							{
								FIRGBAF * pDibLine = (FIRGBAF *)FreeImage_GetScanLine(bitmap, i);
								float * pIplLine = (float *)(pImg->imageData + (imageHeight - i - 1) * pImg->widthStep);//第height-i-1行的行首
								for(unsigned int j = 0; j < imageWidth; ++j)
								{
									pIplLine[4 * j + 0] = pDibLine[j].blue;
									pIplLine[4 * j + 1] = pDibLine[j].green;
									pIplLine[4 * j + 2] = pDibLine[j].red;
									pIplLine[4 * j + 3] = pDibLine[j].alpha;
								}
							}

							images.push_back(pImg); 

							bRet = true;
							break;
						}

					default:
						{
							bRet = false;
							break;
						}
				}

				return bRet;
			};

			std::vector<IplImage *> images;
	};
};

#endif
