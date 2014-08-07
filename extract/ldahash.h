#ifndef _LDAHASH
#define _LDAHASH

//this file is an implementation of ldahash algorithm 

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <xmmintrin.h>
#include "hashpro.h"

#define BIN_WORD unsigned long long
#define SIFT     0
#define DIF128   1
#define LDA128   2
#define DIF64    3
#define LDA64    4
#define TEST	 5
#define TEXTURE  6

typedef union F128
{
	__m128 pack;
	float f[4];
} F128;

class Ldahash
{
	unsigned long long b1; //use a double word to represent a 64 bit long descriptor 
	unsigned long long b2;
	unsigned long long singleBitWord[64];

public:
	Ldahash()
	{
		b1=0;
		b2=0;

		// compute words with particular bit turned on
		// singleBitWord[0] :  000000...000001  <=> 1 = 2^0
		// singleBitWord[1] :  000000...000010  <=> 2 = 2^1
		// singleBitWord[2] :  000000...000100  <=> 4 = 2^2
		// singleBitWord[3] :  000000...001000  <=> 8 = 2^3

		singleBitWord[0] = 1;
		for (int i=1; i < 64; i++)
		{
			singleBitWord[i] = singleBitWord[0] << i;
		}
	};

	~Ldahash(){};

	unsigned long long get_b1(){return b1;};
	unsigned long long get_b2(){return b2;};

	/// a.b
	float sseg_dot(const float* a, const float* b, int sz )
	{

		int ksimdlen = sz/4*4;
		__m128 xmm_a, xmm_b;
		F128   xmm_s;
		float sum;
		int j;
		xmm_s.pack = _mm_set1_ps(0.0);
		for( j=0; j<ksimdlen; j+=4 ) {
			xmm_a = _mm_loadu_ps((float*)(a+j));
			xmm_b = _mm_loadu_ps((float*)(b+j));
			xmm_s.pack = _mm_add_ps(xmm_s.pack, _mm_mul_ps(xmm_a,xmm_b));
		}
		sum = xmm_s.f[0]+xmm_s.f[1]+xmm_s.f[2]+xmm_s.f[3];
		for(; j<sz; j++ ) sum+=a[j]*b[j];
		return sum;
	};

	/// c = Ab
	/// A   : matrix
	/// ar  : # rows of A
	/// ald : # columns of A -> leading dimension as in blas
	/// ac  : size of the active part in the row
	/// b   : vector with ac size
	/// c   : resulting vector with ac size
	void sseg_matrix_vector_mul(const float* A, int ar, int ac, int ald, const float* b, float* c)
	{
		for( int r=0; r<ar; r++ )
			c[r] = sseg_dot(A+r*ald, b, ac);
	};

	void run_sifthash(const float *desc, const int method = LDA128)
	{
		float provec[128];

		switch (method) {
			case DIF128 : {
					      sseg_matrix_vector_mul(Adif128, 128, 128, 128, desc, provec);
					      b1 = 0;
					      for(int k=0; k < 64; k++){
						      if(provec[k] + tdif128[k] <= 0.0) b1 |= singleBitWord[k];
					      }
					      b2 = 0;
					      for(int k=0; k < 64; k++){
						      if(provec[k+64] + tdif128[k+64] <= 0.0) b2 |= singleBitWord[k];
					      }
					      break;
				      }
			case LDA128 : {
					      sseg_matrix_vector_mul(Alda128, 128, 128, 128, desc, provec);
					      b1 = 0;
					/*
					      std::cout << "LDA:" << std::endl;
					      for(int i=0;i< 64;i++)
					      {
						      std::cout << provec[i] << "  ";
					      }
					      std::cout << std::endl;
					*/

					      for(int k=0; k < 64; k++){
					//	      std::cout << provec[k] + tlda128[k] << "  ";
						      if(provec[k] + tlda128[k] <= 0.0) b1 |= singleBitWord[k];
					      }
					  //    std::cout << std::endl;
					      b2 = 0;
					      for(int k=0; k < 64; k++){
						      if(provec[k+64] + tlda128[k+64] <= 0.0) b2 |= singleBitWord[k];
					      }
					      break;
				      }
			case DIF64 : {
					     sseg_matrix_vector_mul(Adif64, 64, 128, 128, desc, provec);
					     b1 = 0;
					     for(int k=0; k < 64; k++) {
						     if(provec[k] + tdif64[k]  <= 0.0) b1 |= singleBitWord[k];
					     }
					     break;
				     }
			case LDA64 : {
					     sseg_matrix_vector_mul(Alda64, 64, 128, 128, desc, provec);
					     b1 = 0;
					     for(int k=0; k < 64; k++) {
						     if(provec[k] + tlda64[k]  <= 0.0) b1 |= singleBitWord[k];
					     }
					     break;
				     }
			case TEST:   {

					     int flag[128] = {0};

					     float min = 1, max = 0;
					     int min_f = 0, max_f = 0;
					     for(int n = 0;n < 10;n++)
					     {
						     min = 1;
						     max = 0;
						     for(int i = 0;i < 128;i++)
						     {
							     if(!flag[i] && desc[i] < min) 
							     {
								     min = desc[i];
								     min_f = i;
							     }
							     if(!flag[i] && desc[i] > max)
							     {
								     max = desc[i];
								     max_f = i;
							     }
						     }
						     flag[min_f] = 1;
						     flag[max_f] = 1;
					     }

					     float sum = 0, avg = 0;
					     for(int k=0; k < 128; k++)
					     {
						     if(!flag[k])sum += desc[k];
					     }
					     avg = sum/(128- 2*10);

					     b1 = 0;
					     for(int k=0; k < 64; k++){
						     if(desc[k] > avg) b1 |= singleBitWord[k];
					     }
					     b2 = 0;
					     for(int k=0; k < 64; k++){
						     if(desc[k+64] > avg) b2 |= singleBitWord[k];
					     }
					     break;
				     }
			case TEXTURE:{

                                             b1 = 0;
                                             for(int k=0; k < 64; k++){
                                                     if(desc[k] > desc[k+1]) b1 |= singleBitWord[k];
                                             }
                                             b2 = 0;
                                             for(int k=0; k < 64; k++){
                                                     if(desc[k+64] > desc[(k+64+1)%128]) b2 |= singleBitWord[k];
                                             }
                                             break;	     
				     }
delault :{
		 std::cout << "ldahash: method not known " << std::endl;
	 }
		};
	};
};

#endif
