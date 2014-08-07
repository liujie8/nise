/**********************************************8
#socket class
***********************************************/
#ifndef _COMMON_SOCKET
#define _COMMON_SOCKET

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
	class Socket
	{
		int listenfd;
		int clientfd;
	
	public:
		Socket()
		{
			listenfd = 0;
			clientfd = 0;
		};

		~Socket()
		{};

		bool setTimeout(int connfd, int t_sec, int t_usec, int type = 0)
		{
			struct timeval timeout;

			timeout.tv_sec = t_sec;
			timeout.tv_usec = t_usec;
			
			int res;

			if(type == 0)
				res = setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout.tv_sec, sizeof(struct timeval));
			else
				res = setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout.tv_sec, sizeof(struct timeval));

			if (res < 0)return false;
			return true;
		};

		int connectTo(const char * srv_ip, int srv_port, int t_sec = 0, int u_sec = 0)
		{
			if( (clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		//		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
				return -1;
			}

			struct sockaddr_in    servaddr;

			memset(&servaddr, 0, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons(srv_port);
			if( inet_pton(AF_INET, srv_ip, &servaddr.sin_addr) <= 0){
			//	printf("inet_pton error for %s\n",srv_ip);
				return -1;
			}


			if( connect(clientfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
			//	printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
				return -1;
			}

			return clientfd;
		};

		int getClientfd()
		{
			return clientfd;
		};

		//server socket
		int listenPort(int servport, int maxlistennum = 100)
		{
			if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
				printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
				return -1;
			}
			
			struct sockaddr_in servaddr;

			memset(&servaddr, 0, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
			servaddr.sin_port = htons(servport);

			if( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
				printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
				return -1;
			}

			if( listen(listenfd, maxlistennum) == -1){
				printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
				return -1;
			}

			return listenfd;
		};

		int getListenfd()
		{
			return listenfd;
		};

		int acceptSocket(int t_sec = 0, int t_usec = 0)
		{ 
			int connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

			if(t_sec != 0 || t_usec != 0)
			{
				if(!setTimeout(connfd, t_sec, t_usec))return 0;
			}

			return connfd;
		};

		int sendSocket(int connfd, char * buf, int buf_size, int t_sec = 0, int t_usec = 0)
		{
			int n = 0;
			n = send(connfd, buf, buf_size, 0);

			return n;
		};

		int recvSocket(int connfd, char * buf, int buf_size, int t_sec = 0, int t_usec = 0)
		{
			int n = 0;
			n = recv(connfd, buf, buf_size, 0);
			
			return n;
		};

		void closeSocket(int connfd)
		{
			close(connfd);
		};
		
	};
};
#endif
