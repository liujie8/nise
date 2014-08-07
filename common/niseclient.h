#ifndef _NISE_CLIENT
#define _NISE_CLIENT

#include "socket.h"
#include "nisereply.h"

namespace nise
{
	class NiseClient
	{
	public:
		NiseClient(){};
		~NiseClient(){};

		int queryImage(const char *ip, int port, std::string img_url)
		{
			reply.replytype = REPLY_FAILED;

			std::string cmd("query");

			Socket sock;
			int connfd = 0;

			if((connfd = sock.connectTo(ip, port)) < 0)
			{
				reply.replytype = REPLY_EXTRACTSERVER_FAIL;
				return 0;
			}

			sendString(connfd, cmd);
			sendString(connfd, img_url);
			reply.recvReply(connfd);
			
			return 1;
		};

		int insertImage(const char *ip, int port, std::string img_url, std::string img_pid = "")
		{
			reply.replytype = REPLY_FAILED;

			std::string cmd("insert");
			Socket sock;
                        int connfd = 0;

                        if((connfd = sock.connectTo(ip, port)) < 0)
			{
				reply.replytype = REPLY_EXTRACTSERVER_FAIL;
				return 0;
			}

			sendString(connfd, cmd);
			sendString(connfd, img_url);
			sendString(connfd, img_pid);
			reply.recvReply(connfd);

			return 1;
		};

		int deleteImage(const char *ip, int port, std::string img_pid)
		{
			reply.replytype = REPLY_FAILED;

			std::string cmd("delete");
			Socket sock;
                        int connfd = 0;

                        if((connfd = sock.connectTo(ip, port)) < 0)
			{
				 reply.replytype = REPLY_EXTRACTSERVER_FAIL;
				 return 0;
			}
		
			sendString(connfd, cmd);
			sendString(connfd, img_pid);
			reply.recvReply(connfd);

			return 1;
		};
	
		int sendString(int connfd, std::string &str)
		{
			size_t str_sz = str.size();

			if(send(connfd, (char *)(&str_sz), sizeof(size_t) , 0) < 0)
			{
			//	printf("str_sz: send msg error: %s(errno: %d)\n", strerror(errno), errno);
				return -1;
			}

			if(str_sz > 0)
				if( send(connfd, reinterpret_cast<char *>(&str.at(0)), str_sz * sizeof(str.at(0)) , 0) < 0)
				{
			//		printf("str: send msg error: %s(errno: %d)\n", strerror(errno), errno);
					return -1;
				}

			return str_sz;
		};

		int recvString(int connfd, std::string &str)
		{
			size_t str_sz = 0;

                        if( recv(connfd, (char *)(&str_sz), sizeof(size_t) , 0) < 0)
                        {
                          //      printf("str_sz: recv msg error: %s(errno: %d)\n", strerror(errno), errno);
                                return -1;
                        }

                        if( str_sz > 0 )
			{
				str.resize(str_sz);

                                if( recv( connfd, reinterpret_cast<char *>(&str.at(0)), str_sz * sizeof(str.at(0)) , 0) < 0)
                                {
                            //            printf("str: recv msg error: %s(errno: %d)\n", strerror(errno), errno);
                                        return -1;
                                }
			}

			return str_sz;
		};
		
		NiseReply reply;
	};
};
#endif
