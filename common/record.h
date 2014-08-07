//define the record struct for an image
#ifndef _COMMON_RECORD
#define _COMMON_RECORD

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <bitset>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <asm/ioctls.h>
#include <sys/ioctl.h>

#include "io.h"
#include "config.h"
#include "hamming.h"

namespace nise
{
/*
	typedef unsigned char Chunk; // could be unsigned short or unsigned.
	typedef unsigned Key;        // any plain old data types
	static const unsigned DATA_BIT = 128; // can be divided by CHUNK_BIT

	static const unsigned CHUNK_BIT = sizeof(Chunk) * 8;
	static const unsigned DATA_CHUNK = 128 / CHUNK_BIT;
	static const unsigned DATA_SIZE = DATA_CHUNK * sizeof(Chunk);
	static const unsigned KEY_SIZE = sizeof(Key);
	static const unsigned RECORD_SIZE = DATA_SIZE + KEY_SIZE;
	static const unsigned MAX_SCAN_RESULT = 500;
*/
	struct BitFeature
	{
		Chunk sketch[DATA_CHUNK];
	};

	struct Point 
	{   // represents one record, should be directly readable from disk
		BitFeature desc;
		Key key;
		
		Point(){};
		~Point(){};

		static unsigned size()
		{
			return sizeof(BitFeature) + sizeof(Key);
		};

		void printPoint()
		{
			for (unsigned i = 0; i < 128 / 8; ++i)
			{
				printf("%02X", desc.sketch[i]);
			}
			printf("       %u", key);
			printf("\n");
		};

		bool readPoint(std::ifstream &is, bool desc_key = true)
		{
			if(desc_key)
			{
				is.read((char *)&desc, sizeof(BitFeature));
				is.read((char *)&key, sizeof(Key));
			}
			else
			{
				is.read((char *)&key, sizeof(Key));
				is.read((char *)&desc, sizeof(BitFeature));
			}

			if(is)
                                return true;
                        else
                                return false;

		};

		bool writePoint(std::ofstream &os, bool desc_key = true)
		{
			if(desc_key)
			{
				os.write((char *)&desc, sizeof(BitFeature));
				os.write((char *)&key, sizeof(Key));
			}
			else
			{
				os.write((char *)&key, sizeof(Key));
				os.write((char *)&desc, sizeof(BitFeature));
			}
		
			if(os)
                                return true;
                        else
                                return false;

		};	
	};
	
	struct PointComparator
	{
		unsigned int offset;
		unsigned int keysize;
		
		PointComparator(unsigned int off = 0, unsigned int size = 128)
		{
			offset = off;
			keysize = size;
		};
		~PointComparator(){};
		
		void setOffset(unsigned int off)
		{
			offset = off;
		};
		
		unsigned int getOffset()
		{
			return offset;
		};
	
		void setKeysize(unsigned int size)
		{
			keysize = size;
		};

		unsigned int getKeysize()
		{
			return keysize;
		};

		bool operator() (const Point &p1, const Point &p2)
		{
			Chunk * c1 = (Chunk *) p1.desc.sketch;
			Chunk * c2 = (Chunk *) p2.desc.sketch;

			for(int i=0;i < keysize/CHUNK_BIT; i++)
			{
				int j = (i + offset/CHUNK_BIT) % DATA_CHUNK; 
				if(c1[j] > c2[j]) return false;
				if(c1[j] < c2[j]) return true;
			}
			return false;
		};
	};

	struct Record
	{		
		std::string img_url;   //image url or image pid
		
		std::vector<BitFeature> desc;  //image descriptors
		unsigned long long fingerprint; //image fingerprint

		Record()
		{
			img_url = "";
			desc.clear();
			fingerprint = 0;
		};

		~Record(){};

		int dedup(unsigned thres = 3)
		{
			Hamming hamming;
			std::vector<BitFeature> tmp;
			tmp.clear();
		
			for(int i=0; i < desc.size(); i++)
			{
				if(i == 0)
				{
					tmp.push_back(desc[i]);
					continue;
				}

				int j=0;
				for(j=0; j < tmp.size(); j++)
				{
					if(hamming.popcount((unsigned long long *)desc[i].sketch, (unsigned long long *)tmp[j].sketch) <= thres)break;
				}
				if(j == tmp.size())tmp.push_back(desc[i]);
			}

			int count = desc.size() - tmp.size();
			desc = tmp;
			return count;
		};

		bool readFields(std::ifstream &is)
		{
			img_url.clear();
			desc.clear();

			//read img_url
			size_t url_sz=0;
			is.read((char *)&url_sz,sizeof(size_t));
			img_url.resize(url_sz);
	//		std::cout <<"url_sz:"<< url_sz << std::endl;
			if(url_sz>0)
				is.read((char *)&img_url.at(0),url_sz);
			
			//read descriptors
			size_t desc_sz=0;
			is.read((char *)&desc_sz,sizeof(size_t));
			desc.resize(desc_sz);
	//		std::cout << "desc_sz:" << desc_sz << std::endl;
			if(desc_sz>0)
				is.read(reinterpret_cast<char *>(&desc.at(0)), desc_sz * sizeof(desc.at(0)));

			if(is)
				return true;
			else
				return false;
		};

		bool writeFields(std::ofstream &os)
		{
			//write img_url
			size_t url_sz = img_url.size();
			os.write((char *)&url_sz,sizeof(size_t));
			if(url_sz>0)
				os.write(&img_url[0],url_sz);

			//write descriptors
			size_t desc_sz = desc.size();
			os.write((char *)&desc_sz,sizeof(size_t));
			if(desc_sz>0)
				os.write(reinterpret_cast<const char *>(&desc[0]), desc_sz * sizeof(desc[0]));

			if(os)
                                return true;
                        else
                                return false;
		};

		bool recvRecord(int sockfd)	
		{
			img_url.clear();
			desc.clear();

			char passcode[10] = "record";
			char pass[10] = {0};
			unsigned passlen = 6;
			if(recv(sockfd, pass, passlen, 0) != passlen)
			{
				printf("record passcode: recv msg error: %s(errno: %d)\n", strerror(errno), errno);
				return false;
			}

			pass[passlen] = '\0';
			if(strcmp(passcode, pass) != 0)
			{
				printf("bad passcode\n");
				return false;
			}

			if( recv(sockfd, (char *)(&fingerprint), sizeof(unsigned long long) , 0)  != sizeof(unsigned long long))
			{
				printf("image fingerprint: recv msg error: %s(errno: %d)\n", strerror(errno), errno);
				return false;
			}

			unsigned url_sz = 0;

			if( recv(sockfd, (char *)(&url_sz), sizeof(unsigned) , 0) != sizeof(unsigned))
			{
				printf("img_url_sz: recv msg error: %s(errno: %d)\n", strerror(errno), errno);
				return false;
			}

			if(url_sz > 400)
			{
				printf("url_sz too large : %u\n", url_sz);
				return false;
			}

			img_url.resize(url_sz);

			if( url_sz > 0 )
				if( recv( sockfd, reinterpret_cast<char *>(&img_url.at(0)), url_sz * sizeof(img_url.at(0)) , 0) != url_sz * sizeof(img_url.at(0)))
				{
					printf("img_url: recv msg error: %s(errno: %d)\n", strerror(errno), errno);
					return false;
				}

			unsigned desc_sz = 0;

			if( recv(sockfd, (char *)(&desc_sz), sizeof(unsigned), 0) != sizeof(unsigned))
			{
				printf("desc_sz: recv msg error: %s(errno: %d)\n", strerror(errno), errno);
				return false;
			}

			if(desc_sz > 200)
			{
				printf("desc_sz too large: %u\n", desc_sz);
				return false;
			}

			desc.resize(desc_sz);

			if(desc_sz > 0)
				if( recv(sockfd, reinterpret_cast<char *>(&desc.at(0)), desc_sz * sizeof(desc.at(0)) , 0) != desc_sz * sizeof(desc.at(0)))
				{
					printf("desc: recv msg error: %s(errno: %d)\n", strerror(errno), errno);
					return false;
				}


			return true;

		};

		bool sendRecord(int sockfd)
		{
			char passcode[10] = "record";
			unsigned passlen = strlen(passcode);
			if(send(sockfd, passcode, passlen, 0) != passlen)
                        {
                                printf("record passcode: send msg error: %s(errno: %d)\n", strerror(errno), errno);
                                return false;
                        }

			if(send(sockfd, (char *)(&fingerprint), sizeof(unsigned long long) , 0) != sizeof(unsigned long long))
			{
				printf("image fingerprint: send msg error: %s(errno: %d)\n", strerror(errno), errno);
				return false;
			}

			unsigned url_sz = img_url.size();

			if(send(sockfd, (char *)(&url_sz), sizeof(unsigned) , 0) != sizeof(unsigned))
			{
				printf("img_url_sz: send msg error: %s(errno: %d)\n", strerror(errno), errno);
				return false;
			}

			if(url_sz > 0)
				if( send(sockfd, reinterpret_cast<char *>(&img_url.at(0)), url_sz * sizeof(img_url.at(0)) , 0) != url_sz * sizeof(img_url.at(0)))
				{
					printf("img_url: send msg error: %s(errno: %d)\n", strerror(errno), errno);
					return false;
				}

			unsigned desc_sz = desc.size();

			if(send(sockfd, (char *)(&desc_sz), sizeof(unsigned), 0) != sizeof(unsigned))
			{
				printf("desc_sz: send msg error: %s(errno: %d)\n", strerror(errno), errno);
				return false;
			}

			if(desc_sz > 0)
				if( send(sockfd, reinterpret_cast<char *>(&desc.at(0)), desc_sz * sizeof(desc.at(0)) , 0) !=  desc_sz * sizeof(desc.at(0)))
				{
					printf("desc: send msg error: %s(errno: %d)\n", strerror(errno), errno);
					return false;
				}

			return true;
		};
	};
}
#endif
