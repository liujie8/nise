//url_buffer
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <queue>
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


using namespace std;

std::queue<string> urls;
std::vector<string> srv_ip;
std::vector<int> srv_port;

boost::mutex queue_mutex;

int url_buffer(int connfd)
{
        char len[11];
	char buf[4096];
	memset(len, 0, 11);

        string pids;
	pids.clear();

	system("date");

	cout << "Receiving image urls ..." << endl;

        int n = recv(connfd, len, 10, 0);
        len[n] = 0;

        int size = atoi(len);

	cout << "Size: " << size << endl;

        pids.resize(size);

        int total = 0;

	char * pos = (char *)pids.c_str();
	
	while(size > 0)
	{
		int num = 0;
		if(size >= 4096)
		{
			num = recv(connfd, pos, 4096, 0);
		}
		else
			num = recv(connfd, pos, size, 0);
		
		if(num <=0 )break;

		size -= num;
		pos += num;
		total += num;
	}

	cout << "Real size : " << total << endl;
 
        istringstream is(pids);

        string pid;

	int cnt =0;

	{
		boost::mutex::scoped_lock lock(queue_mutex);
		
		while(is >> pid)
		{
			urls.push(pid);

//			cout << pid << endl;
			cnt ++;
		};
	}
	cout << pid << endl;
	cout << cnt << " urls received. " << endl;

	close(connfd);

        return cnt;
};

int start_urlbuffer_server(int retrieve_port = 46666)
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

        printf("======waiting for urlbuffer client's request======\n");
        while(1){
                if( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
                        printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
                        continue;
                }

		boost::thread thread(boost::bind(url_buffer , connfd));
        }

        close(listenfd);
	cout << "url buffer thread stoped." << endl;
	return 1;
};

int download_and_distribute(char *srv_ip, int srv_port, string &url)
{
	//download
	string cmd("wget");
	string base_dir("/tmp/image");
	string img_dir;
	cmd = cmd + " -P /tmp/image " + url;

	int pos = url.find_last_of("/");

	if(pos != string::npos)
	{
		string sub = url.substr(pos);
		img_dir = base_dir + url.substr(pos);

		string my("9833b71fjw1dw");
		my = "/" + my;
		sub.resize(my.size());

		if(sub != my)
			return 0;
	}

	cout << "Image dir : " << img_dir << endl;

	cout << cmd << endl;

        system(cmd.c_str());

	//distribute
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

        if( send(sockfd, (char *)img_dir.c_str(), img_dir.size(), 0) < 0)
        {
                printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        }
        else
        {
                printf("send msg to server: %s\n", img_dir.c_str());
        }

        close(sockfd);

	return 1;
};

int start_distribute_server(char *ip, int port)
{
	while(1)
	{
	//	sleep(1);
		usleep(500);

		string url;
		url.clear();

		{
			boost::mutex::scoped_lock lock(queue_mutex);

			if(urls.size())
			{
				url = urls.front();
				urls.pop();
			}
		}

		if(url.size())
		{
			download_and_distribute(ip, port, url);
		}
	};

	cout << "distribute thread stoped." << endl;
	return 1;
};


int main(int argc, char ** argv)
{
        if(argc != 2)
        {
                cout << "Usage: url_buffer <urlbuf_config>" << endl;
                return 0;
        }

        ifstream is(argv[1]);
        if(!is)
        {
                cout << "Failed to load " << argv[1] << endl;
                return 0;
        }

        int buf_port;
        is >> buf_port;

        string extractor_ip;
        int extractor_port;

        int cnt =0;
        while(is >> extractor_ip >> extractor_port)
        {
                srv_ip.push_back(extractor_ip);
                srv_port.push_back(extractor_port);
                cnt++;
        }
        cout << cnt << " extractor server info imported." << endl;

	boost::thread * urlbuf_thread = new boost::thread(boost::bind(start_urlbuffer_server, buf_port));
	cout << "urlbuf thread start." << endl;

	boost::thread * distribute_thread = NULL;
	if(cnt>0)
	{
		distribute_thread = new boost::thread(boost::bind(start_distribute_server, (char *)srv_ip[0].c_str(), srv_port[0]));
		cout << "distribute thread start." << endl;
	}
	
	cout << "Server started." << endl;
	
	urlbuf_thread->join();
	
	if(distribute_thread)
		distribute_thread->join();
	
	cout << "Server stoped." << endl;
	return 1;
};

