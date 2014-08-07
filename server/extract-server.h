#ifndef _EXTRACT_SERVER
#define _EXTRACT_SERVER

#include <iostream>
#include <fstream>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/thread/condition.hpp>
#include <boost/threadpool.hpp>

#include <semaphore.h>

#include <time.h>
#include<sys/utsname.h>
#include<unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <string>
#include <sstream>
#include <signal.h>

#include <curl/curl.h>

#include "../extract/feature_extractor.h"

#include "../common/imageloader.h"
#include "../common/record.h"
//#include "../common/socket.h"
#include "../common/eval.h"
//#include "../common/nisereply.h"
#include "../common/niseclient.h"

namespace nise
{
	class ExtractServer
	{
	public:
		ExtractServer()
		{
			if(CURLE_OK == curl_global_init(CURL_GLOBAL_ALL))
			{
			//	curl_global_cleanup();
			};
		};
		
		~ExtractServer()
		{
			curl_global_cleanup();
		};

		boost::thread * start_extract_server(int extract_port = 33333)
		{
			boost::thread * thread = new boost::thread((boost::bind(ExtractServer::extract_server, extract_port)));
			return thread;	
		};

		boost::thread * start_search_server(int search_port = 44444)
                {
                        boost::thread * thread = new boost::thread((boost::bind(ExtractServer::search_server, search_port)));
                        return thread;
                };

		boost::thread * start_control_server(int control_port = 55555)
                {
                        boost::thread * thread = new boost::thread((boost::bind(ExtractServer::control_server, control_port)));
                        return thread;
                };

		static bool setIndexServer(std::string ip, int insert_p, int delete_p, int query_p)
		{
			boost::mutex::scoped_lock lock(thread_mutex);

			index_server_ip = ip;
			insert_port = insert_p;
			delete_port = delete_p;
			query_port = query_p;

			return true;
		};

		static bool setImagePath(std::string path)
		{
			img_path = path;
			return true;
		};

	private:
		static char * time_stamp(bool print = true)
		{
			time_t time_stamp;
			time(&time_stamp);

			char *c = ctime(&time_stamp);

			if(print) printf("%s", c);

			return c;
		};

		static int control_server(int control_port)
		{
			Socket sock;
			int listenfd, connfd;

			signal(SIGPIPE, SIG_IGN);

			if((listenfd = sock.listenPort(control_port)) > 0)
			{
				time_stamp();
				printf("======waiting for control client's request at port %d======\n", control_port);

				while(1){
					if( (connfd = sock.acceptSocket()) == -1){
						continue;
					}

					char buff[4096];
					int n = recv(connfd, buff, 4096, 0);
					buff[n] = '\0';

					time_stamp();
					printf("recv control msg from client: %s\n", buff);

					if(n > 0)
					{
						std::stringstream cmd(buff);
						std::string c;

						cmd >> c;

						bool success = false;

						std::stringstream out;

						if(c == "help") { std::string opt; cmd >> opt; success = help(out, opt);}
						if(c == "info") { success = info(out);}
						if(c == "reset"){ success = reset(out);}
						if(c == "setimgpath"){std::string path; cmd >> path; success = setImagePath(path);}
						if(c == "setindexserv") 
						{
							std::string srv_ip;
							int insert_p, delete_p, query_p;

							if(cmd >> srv_ip >> insert_p >> delete_p >> query_p)
								success = setIndexServer(srv_ip, insert_p, delete_p, query_p);
						}

						if(success)
							out << cmd.str() << " succeed." << std::endl;
						else
							out << cmd.str() << " failed." << std::endl;


						if( send(connfd, out.str().c_str(), out.str().size(), 0) < 0)
						{
							printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
						}

					}
					sock.closeSocket(connfd);
				}
			
			}

			sock.closeSocket(listenfd);

		};

		static bool help(std::stringstream &out, std::string cmd = "")
		{
			if(cmd == "")
			{
				out << "Supported command :" << std::endl;
				out << "info \t reset \t setimgpath \t setindexserv \t help" << std::endl;
			};

			if(cmd == "info")
			{
				out << "Usage: info" << std::endl;
				out << "Func: Get current statistic information of the extract server." << std::endl;
			};


			if(cmd == "reset")
			{
				out << "Usage: reset" << std::endl;
				out << "Func: reset curent statistic info of the extract server." << std::endl;
			};

			if(cmd == "setimgpath")
			{
				out << "Usage: setimgpath <image path>" << std::endl;
				out << "Func: set image path to store image downloaded from remote image server." << std::endl;
			};

			if(cmd == "setindexserv")
			{
				out << "usage: setindexserv <ip> <insert port> <delete port> <query port>" << std::endl;
				out << "Func: set index server for the extract server." << std::endl;
			};

			return true;
		};

		static bool info(std::stringstream &out)
		{
			out << "## system info" << std::endl;

			//system time
			time_t time_stamp;
			time(&time_stamp); //current time

			out << "# system time " << std::endl;
			out << ctime(&time_stamp);
			out << std::endl;

			out << "# cpu info " << std::endl;
			std::string cpuinfo;
			std::ifstream in1("/proc/stat");
			if(std::getline(in1, cpuinfo)) out << cpuinfo << std::endl;
			in1.close();
			out << std::endl;

			out << "# mem info " << std::endl;
			std::string meminfo;
			int count = 4;
			std::ifstream in2("/proc/meminfo");
			while(count-- > 0 && std::getline(in2, meminfo)) out << meminfo << std::endl;
			in2.close();
			out << std::endl;

			out << "# loadavg info" << std::endl;
			std::string loadavg;
			std::ifstream in3("/proc/loadavg");
			if(std::getline(in3, loadavg)) out << loadavg << std::endl;
			in3.close();
			out << std::endl;

			//system uname info
			struct utsname u_name;
			uname(&u_name);

			out << "# uname info " << std::endl;
			out << "sysname \t" << u_name.sysname << std::endl;;
			out << "nodename \t" << u_name.nodename << std::endl;;
			out << "release \t" << u_name.release << std::endl;
			out << "version \t" << u_name.version << std::endl;
			out << "machine \t" << u_name.machine << std::endl;
			out << std::endl;

			out << "## extract server info " << std::endl;
			out << "# extracting thread " << extract_thread << std::endl;
			out << "# extract success " << extract_success << std::endl;
			out << "# extract failure " << extract_failure << std::endl;
			out << std::endl;

			out << "## search server info " << std::endl;
			out << "# searching thread " << search_thread << std::endl;
			out << "# search success " << search_success << std::endl;
			out << "# search failure " << search_failure << std::endl;
			out << std::endl;

			return true;
		};
		
		static bool reset(std::stringstream &out)
		{
			boost::mutex::scoped_lock lock(thread_mutex);

			info(out);

			out << "## reset" << std::endl;
			out << std::endl;

			//reset 
			extract_success = 0;
			extract_failure = 0;

			search_success = 0;
			search_failure = 0;

			out << "## reset done" << std::endl;
			out << std::endl;
			return true;
		};

		static int search_server(int search_port)
                {
                        Socket sock;
                        int    listenfd, connfd;

                        if( (listenfd = sock.listenPort(search_port, 100)) > 0 ){
                                time_stamp();
                                printf("======waiting for searching client's request at port %d ======\n", search_port);

				boost::threadpool::pool tp(30);

				while(1){
					if( (connfd = sock.acceptSocket(3)) == -1){
						// usleep(2000);
						continue;
					}
					//	sem_wait(&sem);
					//      boost::thread thread(boost::bind(ExtractServer::search_service, connfd));
					nise::Timer timer;
			//		boost::thread thread(boost::bind(ExtractServer::search_service, connfd));

					tp.schedule(boost::bind(ExtractServer::search_service, connfd));
					std::cout << "schedule time cost " << timer.elapsed() << " sec" << std::endl;
					//	sem_post(&sem);
				}
			}

			sock.closeSocket(listenfd);
		};

		static int search_service(int connfd)
		{
			{
				boost::mutex::scoped_lock lock(thread_mutex);
				search_thread++;
			}

			std::string cmd;
			std::string img_url, img_file, img_pid;
			cmd = img_url = img_file = img_pid = "";

			bool success = false;
			//NiseClient client;
			NiseReply reply;

			nise::Timer timer;

			nise::Timer timer2;
			float t1=0,t2=0,t3=0,t4=0,t5=0,t6=0;

			signal(SIGPIPE, SIG_IGN);

			char str_len[12];
			memset(str_len, 0, 10);

			char buff[512];
			memset(buff, 0, 512);

			int m = 0, n = 0;
			m = recv(connfd, str_len, 10, 0);
			if(m == 10)
			{
				str_len[m] = '\0';
				int str_size = atoi(str_len);
				if(str_size > 0 && str_size < 512)
					n = recv(connfd, buff, str_size, 0);
			}

			if(n > 0 && n < 512)
			{
				buff[n] = '\0';
			}

			std::stringstream ss(buff);
			ss >> cmd;

			if(cmd == "query")
			{
				ss >> img_url >> img_pid;

				if(img_pid == "")
				{
					get_pid_from_url(img_url, img_pid);
				}

				t1 = timer2.elapsed();

				std::cout << cmd << "  " << img_url << "  " << img_pid <<  std::endl;

			//	sem_wait(&sem);

				timer2.restart();
				int down_success = download_with_curl(img_url, img_file);	
				t2 = timer2.elapsed();

				timer2.restart();

				if(!down_success)
				{
					reply.replytype = REPLY_DOWNLOAD_FAIL;
				}
				else
				{
				
					ImageLoader loader;
					std::cout << "Loading image " << img_file << std::endl;
					int load_success = loader.loadImage(img_file.c_str(), true);
			//		int load_success = 0;
					t3 = timer2.elapsed();
					timer2.restart();

					if(!load_success)
					{
						reply.replytype = REPLY_IMAGELOAD_FAIL;
					}
					else
					{
						std::vector<boost::thread *> thread_pt;
						std::vector<NiseReply> replys;

						int max_image = 10;
						if(loader.size() <= max_image)
						{
							thread_pt.resize(loader.size());
							replys.resize(loader.size());
						}
						else
						{
							thread_pt.resize(max_image);
							replys.resize(max_image);
						}

						for(int i=0;i<replys.size();i++)                                                {
							IplImage *img = loader.getImg(i);
							query_service(img, img_pid, &replys[i]);
						}

				/*
						if(thread_pt.size() == 1)
						{
							IplImage *img = loader.getImg(0);
							query_service(img, img_pid, &replys[0]);
						}
						else
						{
				
						for(int i=0;i < thread_pt.size(); i++)
						{
							IplImage *img = loader.getImg(i);
							thread_pt[i] = NULL;

							thread_pt[i] = (boost::thread *)new boost::thread(boost::bind(ExtractServer::query_service, img, img_pid, &replys[i]));
						}

						//wait until thread finished
						for(int i=0;i < thread_pt.size(); i++)
						{
							if(thread_pt[i])
							{
								thread_pt[i]->join();
								delete thread_pt[i];
							}
						}
				
						}
					*/

						reply.replytype = replys[0].replytype;

						for(int i=0; i < replys.size(); i++)
							reply.pidscore.insert(reply.pidscore.end(), replys[i].pidscore.begin(), replys[i].pidscore.end());
						std::sort(reply.pidscore.begin(), reply.pidscore.end(), ExtractServer::cmpPidScore);
					}

					loader.release();
					t4 = timer2.elapsed();
					timer2.restart();
				//	delete_image(img_file);
					t5 = timer2.elapsed();
				
				}
			//	sem_post(&sem);
			}

			timer2.restart();
			reply.sendStringReply(connfd);
			close(connfd);

			t6 = timer2.elapsed();

			if(reply.replytype == REPLY_OK)success = true;

			nise::Time time;
			char log_file[100];

			if(success)
			{
				sprintf(log_file, "extract_logs/%s_%s_success", time.curdate(), cmd.c_str());
			}
			else
			{
				sprintf(log_file, "extract_logs/%s_%s_failure", time.curdate(), cmd.c_str());
			}

			std::ofstream out(log_file, std::ios::app);

			out << time.curtime() << "\t" << img_url << "\t" << img_file << "\t" << timer.elapsed() << " sec"<< "\t" << reply.replycode() << std::endl  << t1 << "\t" << t2 << "\t" << t3 << "\t" << t4 << "\t" << t5 << "\t"<< t6 << std::endl;
			out.close();
		
			{
				boost::mutex::scoped_lock lock(thread_mutex);
				search_thread--;

				if(success)
				{
					search_success++;
				}
				else
				{
					search_failure++;
				}
			}

			return 1;
		};


		static int extract_server(int extract_port)
		{
			Socket sock;
			int    listenfd, connfd;

			if( (listenfd = sock.listenPort(extract_port, 100)) > 0 ){
				time_stamp();
				printf("======waiting for feature extracting client's request at port %d ======\n", extract_port);

				while(1){
					if( (connfd = sock.acceptSocket(3)) == -1){
						continue;
					}

					boost::thread thread(boost::bind(ExtractServer::extract_service, connfd));
				}
			}

			sock.closeSocket(listenfd);
		};


		static int extract_service(int connfd)
		{
			{
				boost::mutex::scoped_lock lock(thread_mutex);
				extract_thread++;
			}

			std::string cmd;
			std::string img_url, img_file, img_pid;
			cmd = img_url = img_file = img_pid = "";

			bool success = false;
			//NiseClient client;
			NiseReply reply;

			nise::Timer timer;

			signal(SIGPIPE, SIG_IGN);

			char str_len[12];
			memset(str_len, 0, 10);

			char buff[512];
			memset(buff, 0, 512);

			int m = 0, n = 0;
			m = recv(connfd, str_len, 10, 0);
			if(m == 10)
			{
				str_len[m] = '\0';
				int str_size = atoi(str_len);
				if(str_size > 0 && str_size < 512)
					n = recv(connfd, buff, str_size, 0);
			}

			if(n > 0 && n < 512)
			{
				buff[n] = '\0';
			}

			std::stringstream ss(buff);
			ss >> cmd;

			if(cmd == "insert")
			{
				ss >> img_url >> img_pid;

				if(img_pid == "")
				{
					get_pid_from_url(img_url, img_pid);
				}

				std::cout << cmd << "  " << img_url << "  " << img_pid <<  std::endl;

				if(!download_with_curl(img_url, img_file))
				{
					reply.replytype = REPLY_DOWNLOAD_FAIL;
				}
				else
				{
					ImageLoader loader;
					std::cout << "Loading image " << img_file << std::endl;

					if(!loader.loadImage(img_file.c_str(), true))
					{
						reply.replytype = REPLY_IMAGELOAD_FAIL;
					}
					else
					{
						std::vector<boost::thread *> thread_pt;
						std::vector<NiseReply> replys;

						int max_image = 10;
						if(loader.size() <= max_image)
						{
							thread_pt.resize(loader.size());	
							replys.resize(loader.size());
						}
						else
						{
							thread_pt.resize(max_image);
							replys.resize(max_image);
						}

						for(int i=0;i < thread_pt.size(); i++)
						{
							IplImage *img = loader.getImg(i);
							thread_pt[i] = NULL;

							thread_pt[i] = (boost::thread *)new boost::thread(boost::bind(ExtractServer::insert_service, img, img_pid, &replys[i]));
						}

						//wait until thread finished
						for(int i=0;i < thread_pt.size(); i++)
						{
							if(thread_pt[i])
							{
								thread_pt[i]->join();
								delete thread_pt[i];
							}
						}

						reply.replytype = replys[0].replytype;
					}

					loader.release();
					delete_image(img_file);
				}
			}

			if(cmd == "delete")
			{
				ss >> img_pid;
				std::cout << cmd << "  " << img_pid << std::endl;
				delete_service(img_pid, &reply);
			}

			reply.sendStringReply(connfd);

			close(connfd);

			{
				boost::mutex::scoped_lock lock(thread_mutex);
				extract_thread--;

				if(reply.replytype == REPLY_OK)success = true;

				nise::Time time;
				char log_file[100];

				if(success)
				{
					extract_success++;
					sprintf(log_file, "extract_logs/%s_%s_success", time.curdate(), cmd.c_str());
				}
				else
				{
					extract_failure++;
					sprintf(log_file, "extract_logs/%s_%s_failure", time.curdate(), cmd.c_str());
				}

				std::ofstream out(log_file, std::ios::app);

				if(cmd == "insert")
					out << time.curtime() << "\t" << img_url << "\t" << img_file << "\t" << img_pid << "\t" << timer.elapsed() << " sec"<< "\t" << reply.replycode() << std::endl;
				else if(cmd == "delete")
					out << time.curtime() << "\t" << img_pid << "\t" << timer.elapsed() << " sec"<< "\t" << reply.replycode() << std::endl;
				out.close();
			}

			return 1;
		};

		static int insert_service(IplImage *img, std::string img_pid, NiseReply *reply)
		{
			Record record;
			record.img_url = img_pid;

			if(!extractFeature(img, record, 40))
			{
				reply->replytype = REPLY_FEATEXTRACT_FAIL;
				return 0;
			};
			
			Socket sock;
			int connfd = 0;
			if((connfd = sock.connectTo(index_server_ip.c_str(), insert_port)) > 0)
			{
				record.sendRecord(connfd);
				reply->recvReply(connfd);
				sock.closeSocket(connfd);
			}
			else
			{
				reply->replytype = REPLY_INDEXSERVER_FAIL;
				return -1;
			};

			return 1;
		};

		static int delete_service(std::string img_pid, NiseReply *reply)
		{
			Record record;
			record.img_url = img_pid;

                        Socket sock;
                        int connfd = 0;
                        if((connfd = sock.connectTo(index_server_ip.c_str(), delete_port)) > 0)
                        {
                                record.sendRecord(connfd);
                                reply->recvReply(connfd);
				sock.closeSocket(connfd);
                        }
			else
			{
				reply->replytype = REPLY_INDEXSERVER_FAIL;
				return -1;
			};

			return 1;
		};

		static int query_service(IplImage *img, std::string img_pid, NiseReply *reply)
		{
			Record record;
			record.img_url = img_pid;

			nise::Timer timer;
			float t1=0,t2=0,t3=0;
			
			if(!extractFeature(img, record, 60, false))
			{
				reply->replytype = REPLY_FEATEXTRACT_FAIL;
				return 0;
			};
		

			t1 = timer.elapsed();
			timer.restart();

			Socket sock;
                        int connfd = 0;
		
                        if((connfd = sock.connectTo(index_server_ip.c_str(), query_port)) > 0)
                        {
				t2 = timer.elapsed();
				timer.restart();

                                record.sendRecord(connfd);
                                reply->recvReply(connfd);
				t3 = timer.elapsed();
	//			sock.closeSocket(connfd);
                        }
			else
			{
				reply->replytype = REPLY_INDEXSERVER_FAIL;
			//	return -1;
			};
			sock.closeSocket(connfd);
		
			std::cout << t1 << "\t" << t2 << "\t" << t3 << std::endl;
			return 1;
		};

		static int extractFeature(IplImage *img, nise::Record &record, int max_feat = 80, bool fingerprint = true)
		{
			if(!img)return 0;
			record.desc.clear();
			
			if(img->depth != IPL_DEPTH_8U)return 0;

			FeatureExtractor xtor;
			xtor.setImage(img);
			if(fingerprint)record.fingerprint = xtor.getFingerPrint();
			xtor.getBitDesc(record.desc, LDATEST, max_feat);
			record.dedup(7);
			return 1;
		};

		static bool cmpPidScore(const PidScore &ps1, const PidScore &ps2)
		{
			return ps1.score > ps2.score;
		};

		static int get_pid_from_url(std::string &url, std::string &pid)
		{
			std::string::size_type pos = url.find_last_of('/');
                        if (pos == std::string::npos) return 0;
			else
				pid = url.substr(pos + 1);
		
			pos = pid.find_first_of('.');
                        if (pos != std::string::npos)
                                pid = pid.substr(0, pos);
		
			pos = pid.find_first_of('?');
                        if (pos != std::string::npos)
                                pid = pid.substr(0, pos);

			return pid.size();
		};

		static int mkdirs(const char *sPathName)  
		{  
			char   DirName[256];  
			strcpy(DirName,   sPathName);  
			int   i,len   =   strlen(DirName);  
			if(DirName[len-1]!='/')  
				strcat(DirName,   "/");  

			len = strlen(DirName);  

			for(i=1; i<len; i++)  
			{  
				if(DirName[i]=='/')  
				{  
					DirName[i] = 0;  
					if(access(DirName, NULL) != 0)  
					{  
						if(mkdir(DirName, 0777) == -1)  
						{   
							return   -1;   
						}
					}
					DirName[i]   =   '/';
				}
			}

			return 0;
		};


		static size_t write_data(void * buffer, size_t size, size_t nmemb, void * user_p)
		{
			if (NULL == user_p)
				return 0;
			return fwrite(buffer, size, nmemb, (FILE *)user_p);
		}

		static int download_with_curl(std::string & url, std::string & img_file)
		{
			img_file = "";
			get_pid_from_url(url, img_file);
			if(!img_file.size())return 0;

			//if(false)
			//	{
			nise::Time time;
			std::string path;

			path = img_path + "/" + time.curdir(5);

			DIR *dir;
			dir = opendir(path.c_str());
			if(dir == NULL)
			{
				int status = mkdirs(path.c_str());
			}
			else
				closedir(dir);
			//	}

			img_file = path + '/' + img_file;

                      //  img_file = img_path + '/' + img_file;

			CURL * curl = curl_easy_init();
			if (NULL == curl)return 0;

			FILE * fp = fopen(img_file.c_str(), "wb");
			if (NULL == fp) {
				curl_easy_cleanup(curl);
				return 0;
			}

			char error_buffer[CURL_ERROR_SIZE] = {'\0'};
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ExtractServer::write_data);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
			curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);

			CURLcode ret = curl_easy_perform(curl);
			fclose(fp);
			curl_easy_cleanup(curl);

			if (CURLE_OK == ret) {
				return 1;
			} 
			else 
			{
				return 0;
			}
		}


		static int download_with_wget(std::string & url, std::string & img_file)
		{
/*			std::string::size_type pos = url.find_last_of('/');
			if (pos == std::string::npos) {
				time_stamp();
				std::cout << "[download_with_wget] error: invalid url: " << url << std::endl;
				return 0;
			}
			img_file = url.substr(pos);
			img_file = img_path + img_file;
*/
			img_file = "";
                        get_pid_from_url(url, img_file);
                        if(!img_file.size())return 0;

                        nise::Time time;
                        std::string path;

                        path = img_path + "/" + time.curdir(5);

                        DIR *dir;
                        dir = opendir(path.c_str());
                        if(dir == NULL)
                        {
                                int status = mkdirs(path.c_str());
                        }
                        else
                                closedir(dir);

                        img_file = path + '/' + img_file;

			std::string dl_cmd = "wget -T 5 -q -O ";
			dl_cmd += img_file;
			dl_cmd += + " ";
			dl_cmd += url;

			int nRet = system(dl_cmd.c_str());
			if (-1 == nRet || 127 == nRet) {
				time_stamp();
				std::cout << "[download_with_wget] error: system call error to exec : " << dl_cmd << std::endl;
				return 0;
			}

			return 1;
		};


		static int delete_image(std::string & img_file)
		{
			std::string cmd = "sudo rm ";
			cmd += img_file;

			int nRet = system(cmd.c_str());

			if (-1 == nRet || 127 == nRet) {
				time_stamp();
				std::cout << "[delete image] error: system call error to exec : " << cmd << std::endl;
				return 0;
			}

			return 1;
		};

		//image path
		static std::string img_path;

		static boost::mutex thread_mutex;
		static int extract_thread;
		static int extract_success;
		static int extract_failure;
		static int search_thread;
		static int search_success;
		static int search_failure;

		static std::string index_server_ip;
		static int delete_port;
		static int insert_port;
		static int  query_port;
	};
};
#endif
