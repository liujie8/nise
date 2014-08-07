#ifndef _NISE_REPLY
#define _NISE_REPLY
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
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

namespace nise
{
	typedef unsigned Key;

	//for result rank
	struct KeyScore
	{
		Key key;
		float score;

		KeyScore()
		{
			reset();
		};

		~KeyScore(){};

		void reset()
		{
			key = 0;
			score = 0;
		};
	};

	struct PidScore
	{
		std::string pid;
		float score;

		PidScore()
		{
			reset();
		};

		~PidScore(){};

		void reset()
		{
			pid = "";
			score = 0;
		};
	};

	//search result reply class
	enum ReplyType{REPLY_OK, REPLY_FAILED, REPLY_DOWNLOAD_FAIL, REPLY_IMAGELOAD_FAIL, REPLY_FEATEXTRACT_FAIL, REPLY_EXTRACTSERVER_FAIL, REPLY_INDEXSERVER_FAIL, REPLY_INVALID_PID, REPLY_REDISSERVER_FAIL};

	class NiseReply
	{
		public:	
		ReplyType replytype;
		//std::vector<KeyScore> keyscore;
		std::vector<PidScore> pidscore;

		NiseReply()
		{
			replytype = REPLY_FAILED; 
		};

		~NiseReply(){};

		std::string replycode()
		{
			std::string code;

			switch(replytype)
			{
				case REPLY_OK : code = "REPLY_OK"; break;
				case REPLY_FAILED : code = "REPLY_FAILED"; break;
				case REPLY_DOWNLOAD_FAIL : code = "REPLY_DOWNLOAD_FAIL"; break;
				case REPLY_IMAGELOAD_FAIL : code = "REPLY_IMAGELOAD_FAIL"; break; 
				case REPLY_FEATEXTRACT_FAIL : code = "REPLY_FEATEXTRACT_FAIL"; break; 
				case REPLY_EXTRACTSERVER_FAIL : code = "REPLY_EXTRACTSERVER_FAIL"; break;
				case REPLY_INDEXSERVER_FAIL : code = "REPLY_INDEXSERVER_FAIL"; break;
				case REPLY_INVALID_PID : code = "REPLY_INVALID_PID"; break;
				case REPLY_REDISSERVER_FAIL : code = "REPLY_REDISSERVER_FAIL"; break;
				default:
					return "";
			};

			return code;
		};


		bool sendReply(int connfd)
		{
			if(send(connfd, (char *)(&replytype), sizeof(ReplyType) , 0) != sizeof(ReplyType))
			{
				printf("reply type: send msg error: %s(errno: %d)\n", strerror(errno), errno);
				return false;
			}

			std::stringstream str;

			str.str().clear();

			for(int i=0; i < pidscore.size(); i++)
			{
				str << pidscore[i].pid << "\t" <<  pidscore[i].score << "\n";
			}

			unsigned str_size = str.str().size();

			if(send(connfd,(char *)&str_size, sizeof(unsigned), 0) > 0 && str_size > 0)
			{
				unsigned n = send(connfd, (char *)(&str.str().at(0)), str_size * sizeof(str.str().at(0)), 0);
			//	printf("str size %d \t send bytes %d\n", str_size, n);
				if(n != str_size)
					printf("Failed to send results to client.\n");
			}

			return true;
		};

		bool sendStringReply(int connfd)
		{
			std::stringstream str;

			str.str().clear();

			str << replycode() << "\t" << pidscore.size() << "\t";
			for(int i=0; i < pidscore.size(); i++)
			{
				str << pidscore[i].pid << "\t" <<  pidscore[i].score << "\t";
			}
			
			str << "\n";

			int str_size = str.str().size();

			char str_len[12];
			sprintf(str_len,"%010d", str_size);
			str_len[10]='\0';

			int m = send(connfd, str_len, 10, 0);
			if(m == 10 && str_size > 0)
                        {
				int n = send(connfd, (char *)(&str.str().at(0)), str_size * sizeof(str.str().at(0)), 0);
			
				if(n != str_size)
				{
					printf("Failed to send results to client.\n");
					return false;
				}
			}

			return true;

		};


		bool recvReply(int connfd)
		{
			if(recv(connfd, (char *)(&replytype), sizeof(ReplyType) , 0) != sizeof(ReplyType))
			{
				printf("reply type: recv msg error: %s(errno: %d)\n", strerror(errno), errno);
				return false;
			}
			
			std::string str;
			
			unsigned str_size = 0;

			if(recv(connfd, (char *)&str_size, sizeof(unsigned), 0) == sizeof(unsigned) && str_size > 0)
			{
				str.resize(str_size);

				unsigned n = recv(connfd, (char *)(&str.at(0)), str_size * sizeof(str.at(0)), 0);
			//	printf("str size %d \t recv bytes %d\n", str_size, n);
			//	printf("%s", (char *)str.str().c_str());
				if(n != str_size)
				{
					printf("Failed to recv results to client.\n");
				}
				else
				{
					std::stringstream buff(str);
					PidScore result;
				//	str >> result.pid >> result.score;
				//	std::cout << result.pid << result.score << std::endl;
					//int count = 0;
					while(buff >> result.pid >> result.score)
					{
					//	std::cout << "result " << ++count << std::endl;
						pidscore.push_back(result);
					};

				}
			}

			return true;
		};
	};

};

#endif
