//extractor.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>

//boost
#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/thread/condition.hpp>

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

#include "../common/record.h"
#include "../common/eval.h"
#include "feature_extractor.h"

using namespace std;
using namespace nise;

//global var
std::vector<std::string> srv_ip;
std::vector<int> srv_port;
boost::mutex io_mutex;


int index(Record &record, std::string *res/*vector<Key> &result*/, char * srv_ip, int srv_port)
{
	string &result = *res;

	int    sockfd, n;
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
	
	record.sendRecord(sockfd);

	close(sockfd);
	return 1;
};

int getPid(std::string &url)
{
        if(url.size()==0)return 0;

        char * pos1 = strrchr(url.c_str(), '/');

        if(pos1 == NULL)return 0;

        char * pos2 = strchr(pos1, '.');
        if(pos2==NULL) pos2 = strrchr(pos1, '&');
        if(pos2 == NULL)
	{
		url = std::string(pos1+1);
		return 0;
	}

        url = std::string(pos1+1, pos2);
        return 1;
};

int extract(int connfd)
{
	char buff[4096];
	memset(buff, 0, 4096);
	int n = recv(connfd, buff, 4096, 0);
	
	if(n > 0)
	{
		std::string src(buff);

		FeatureExtractor xtor;
		Record record;
		std::vector<Key> result[10];
		std::string res[10];
		boost::thread *thread[10];

		if(xtor.loadImage(src))
		{
			cout << "Extracting ..." << endl;
			Timer timer;
			xtor.getBitDesc(record.desc, LDAHASH, false);
			record.img_url = src;
			getPid(record.img_url);

			cout << "Time costs :" << timer.elapsed() << " seconds." << endl;
	
			
			int srv_num = srv_ip.size();

			cout << "Indexing ..." << endl;

			timer.restart();
		
			for(int i=0;i < srv_num; i++)
				thread[i] = (boost::thread *) new boost::thread(boost::bind(index, record, &res[i], (char *)srv_ip[i].c_str(), srv_port[i]));

			for(int i=0;i < srv_num; i++)
				thread[i]->join();

			cout << "Time costs: " << timer.elapsed() << " seconds." << endl;
		}
		else
			std::cout << "Failed to load image " << src << std::endl;
	}
	
	close(connfd);
	return 1;
};

int start_extract_server(int retrieve_port = 56666)
{
	int    listenfd, connfd;
	struct sockaddr_in     servaddr;

	if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
		printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
		exit(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(retrieve_port);

	if( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
		exit(0);
	}

	if( listen(listenfd, 20) == -1){
		printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
		exit(0);
	}

	printf("======waiting for extractor client's request======\n");
	while(1){
		if( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
			printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
			continue;
		}

		boost::thread thread(boost::bind(extract , connfd));
	}

	close(listenfd);	
};

int start_control_server(int control_port)
{
};


int main(int argc, char ** argv)
{
	if(argc != 2)
	{
		cout << "Usage: extractor <xtor_config>" << endl;
		return 0;
	}
	
	ifstream is(argv[1]);
	if(!is)
	{
		cout << "Failed to load " << argv[1] << endl;
		return 0;
	}

	int xtor_port;
	is >> xtor_port;

	string index_ip;
	int index_port;

	int cnt =0;
	while(is >> index_ip >> index_port)
	{
		srv_ip.push_back(index_ip);
		srv_port.push_back(index_port);
		cnt++;
	}
	cout << cnt << " index server info imported." << endl;
	start_extract_server(xtor_port);
	return 1;
};
