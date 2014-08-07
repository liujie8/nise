//extract client
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

#include <iostream>
#include <vector>
#include <string>

#include "../common/filename.h"

using namespace std;
using namespace nise;

int extract_client(char * srv_ip, int srv_port, const char * command)
{
	int    sockfd, n;
	char    recvline[4096], sendline[4096];
	struct sockaddr_in    servaddr;

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(srv_port);
	if( inet_pton(AF_INET, srv_ip, &servaddr.sin_addr) <= 0){
		printf("inet_pton error for %s\n",srv_ip);
		return 0;
	}


	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		return 0;
	}

	if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
		printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
		return 0;
	}

	if( send(sockfd, command, strlen(command), 0) < 0)
	{
		printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
	}
	else
	{
		printf("send msg to server: %s\n", command);
/*
		if( recv(sockfd, recvline, 4096, 0) > 0)
		{
			printf("********** server stat ***********\n");
			printf("%s", recvline);
		}
*/
	}

	close(sockfd);
	return 1;
};

int main(int argc, char ** argv)
{
	if(argc != 4)
	{
		printf("Usage: extract_client <srv_ip> <srv_port> <base_dir>\n");
		return 0;
	}
	FileName filename;
	filename.setDir(argv[3]);

	vector<string> files;

	cout << "Reading files ..." << endl;
	
	filename.getFiles(files);
	
	cout << files.size() << " files read from " << argv[3] << endl;
	
	cout << "Sending files ..." << endl;
	for(int i=0; i < files.size();i++)
	{
		sleep(1);
		cout << i << ":\t" << files[i] << endl;
		extract_client(argv[1], atoi(argv[2]), files[i].c_str());
	}

	cout << "Finished !" << endl;

	return 1;
};

