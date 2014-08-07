#ifndef _INDEX_SERVER
#define _INDEX_SERVER
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/thread/condition.hpp>
#include <boost/threadpool.hpp>

#include <time.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <signal.h>

#include "../common/eval.h"
#include "../common/record.h"
#include "../common/key.h"
#include "../common/socket.h"
#include "../common/nisereply.h"
#include "../index/multi-index.h"

#define _LOG 1

namespace nise
{
	class IndexServer
	{
		public:

			IndexServer()
			{
			
			};

			~IndexServer()
			{
				stop();
			};

			boost::thread * start_control_server(int control_port = 22222)
			{
				boost::thread * thread = new boost::thread((boost::bind(IndexServer::control_server, control_port)));
				return thread;
			};

			boost::thread * start_delete_server(int delete_port = 11111)
			{
				boost::thread * thread = new boost::thread((boost::bind(IndexServer::delete_server, delete_port)));
				return thread;
			};

                        boost::thread * start_insert_server(int insert_port = 9999)
                        {
                                boost::thread * thread = new boost::thread((boost::bind(IndexServer::insert_server, insert_port)));
                                return thread;
                        };

			boost::thread * start_query_server(int query_port = 8888)
			{
				boost::thread * thread = new boost::thread((boost::bind(IndexServer::query_server, query_port)));
				return thread;
			};

			void stop()
			{

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

				if((listenfd = sock.listenPort(control_port)) > 0)
				{
					time_stamp();		
					printf("======waiting for control client's request at port %d======\n", control_port);

					while(1){
						if( (connfd = sock.acceptSocket()) == -1){
							continue;
						}						                                
						signal(SIGPIPE, SIG_IGN);
		
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
					out << "info \t reset \t help" << std::endl;
				};

				if(cmd == "info")
				{
					out << "Usage: info" << std::endl;
					out << "Func: Get current statistic information of the index server." << std::endl;
				};


				if(cmd == "reset")
				{
					out << "Usage: reset" << std::endl;
					out << "Func: reset curent statistic info of the index server." << std::endl;
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

				out << "## server stats" << std::endl;
				out << "# insert server info :" << std::endl;
				out << "inserting thread : " << insert_thread << std::endl;
				out << "insert success : " << insert_success << std::endl;
				out << "insert failure : " << insert_failure << std::endl;
				out << std::endl;
				out << "# delete server info :" << std::endl;
				out << "deleting thread : " << delete_thread << std::endl;
                                out << "delete success : " << delete_success << std::endl;
                                out << "delete failure : " << delete_failure << std::endl;
				out << std::endl;
				out << "# query server info :" << std::endl;
				out << "querying thread : " << query_thread << std::endl;
                                out << "query success : " << query_success << std::endl;
                                out << "query failure : " << query_failure << std::endl;
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
				insert_success = 0;
				insert_failure = 0;
				delete_success = 0;
				delete_failure = 0;
				query_success = 0;
				query_failure = 0;

				out << "## reset done" << std::endl;
				out << std::endl;
				return true;
			};

			static int insert_server(int insert_port)
			{
				Socket sock;
				int    listenfd, connfd;

				if( (listenfd = sock.listenPort(insert_port)) > 0 ){

					time_stamp();

					printf("======waiting for insert client's request at port %d======\n", insert_port);

					while(1){
						if( (connfd = sock.acceptSocket(1)) == -1){
							continue;
						}

						boost::thread thread(boost::bind(IndexServer::insert_service, connfd));
					}
				}

				sock.closeSocket(listenfd);
			};

			static int insert_service(int connfd)
			{
				{
					boost::mutex::scoped_lock lock(thread_mutex);
					insert_thread++;
				}

				bool success = false;

				signal(SIGPIPE, SIG_IGN);

				Record record;
				record.recvRecord(connfd);

				NiseReply reply;
				reply.replytype = REPLY_REDISSERVER_FAIL;
				
				time_stamp();
				std::cout << "inserting image " << record.img_url << std::endl;

				nise::Timer timer;

				KeyAllocator keyalloc;
				Key imageid = 0;
				
				bool valid_pid = false;
				if(record.img_url.size())valid_pid = true;
				else
					reply.replytype = REPLY_INVALID_PID;

				if(valid_pid && keyalloc.redisConnect())
				{
					MultiIndex mindex;
					if(mindex.redisConnect())
					{
					//	Key imageid = 0;
						int count = 100;
						while(count-- > 0)
						{
							imageid = keyalloc.allocateKey();
							if(imageid > 0 && !mindex.checkImageID(imageid))break;
							imageid = 0;
						}

						if(imageid > 0)
						{
							ImageAttrib attrib;
							attrib.attribOn(PID);
							attrib.attribOn(FINGERPRINT);
							attrib.attribOn(SIFTNUM);

							attrib.pid = record.img_url;
							attrib.fingerprint = record.fingerprint;
							attrib.siftnum = record.desc.size();

							mindex.insertImageAttrib(imageid, attrib);
							mindex.insertRecord(record, imageid);
							mindex.insertFingerPrint(record.fingerprint, imageid);
							mindex.insertImagePID((char *)record.img_url.data(),record.img_url.size(), imageid);

							success = true;
							reply.replytype = REPLY_OK;
							std::cout << "image id " << imageid << std::endl;
						}
						else
						{
							std::cout << "failed to alloc image id." << std::endl;
						}
					
					}
				}

				printf("Time cost : %f sec\n", timer.elapsed());

				reply.sendReply(connfd);

				close(connfd);

				{
					boost::mutex::scoped_lock lock(thread_mutex);
					insert_thread--;

					nise::Time time;
					char log_file[100];
					
					if(success)
					{
						insert_success++;
						sprintf(log_file, "index_logs/%s_insert_success", time.curdate());
					}
					else
					{
						insert_failure++;
						sprintf(log_file, "index_logs/%s_insert_failure", time.curdate());
					}

					std::ofstream out(log_file, std::ios::app);
                                        out << time.curtime() << "\t" << imageid << "\t" << record.img_url << "\t" << record.fingerprint << "\t" << record.desc.size() << "\t" << timer.elapsed() << " sec"<< "\t" << reply.replycode() << std::endl;

				}

				return 1;
			};


			static int delete_server(int delete_port)
			{
				Socket sock;
				int listenfd, connfd;

				if( (listenfd = sock.listenPort(delete_port)) > 0 ){
				
					time_stamp();
					printf("======waiting for delete client's request at port %d======\n", delete_port);

					while(1){
						if( (connfd = sock.acceptSocket(1)) == -1){
							continue;
						}

						boost::thread thread(boost::bind(IndexServer::delete_service, connfd));
					}
				}

				sock.closeSocket(listenfd);
			};

			static int delete_service(int connfd)
			{
				{
					boost::mutex::scoped_lock lock(thread_mutex);
					delete_thread++;
				}

				bool success = false;

				signal(SIGPIPE, SIG_IGN);

				Record record;
				record.recvRecord(connfd);
				
				NiseReply reply;
				reply.replytype = REPLY_REDISSERVER_FAIL;

				time_stamp();
				std::cout << "deleting image " << record.img_url << std::endl;

				nise::Timer timer;

				KeyAllocator keyalloc;
				int imageid_sz = 0;

				if(keyalloc.redisConnect())
				{
					MultiIndex mindex;
					if(mindex.redisConnect())
					{
						std::vector<Key> imageid;

						mindex.queryImagePID((char *)record.img_url.data(),record.img_url.size(), imageid);
						std::cout << "Image id size : " << imageid.size() << std::endl;

						imageid_sz = imageid.size();

						if(imageid.size())
						{
							std::cout << "delete pid" << std::endl;
							mindex.deleteImagePID((char *)record.img_url.data(), record.img_url.size(), 0, true);

							for(int i=0; i < imageid.size(); i++)
							{
								std::vector<Point> sketch;

								std::cout << "get sketch " << std::endl;
								mindex.getSketch(imageid[i], sketch);

								std::cout << "delete points" << std::endl;
								for(int j=0; j < sketch.size(); j++)
								{
									mindex.deletePoint(sketch[j]);
								}

								std::cout << "delete fingerprint" << std::endl;
								unsigned long long fp = mindex.getFingerPrint(imageid[i]);
								if(fp)mindex.deleteFingerPrint(fp, imageid[i]);

								std::cout << "delete attrib" << std::endl;
								mindex.deleteImageAttrib(imageid[i]);

								std::cout << "recycle key " << imageid[i] << std::endl;
								keyalloc.recycleKey(imageid[i]);
							}

							success = true;
							reply.replytype = REPLY_OK;
						}
						else
							reply.replytype = REPLY_INVALID_PID;

					}
				}

				printf("Time cost : %f sec\n", timer.elapsed());

				reply.sendReply(connfd);

				close(connfd);

				{
					boost::mutex::scoped_lock lock(thread_mutex);
					delete_thread--;
					
					nise::Time time;
                                        char log_file[100];

                                        if(success)
                                        {
                                                delete_success++;
                                                sprintf(log_file, "index_logs/%s_delete_success", time.curdate());
                                        }
                                        else
                                        {
                                                delete_failure++;
                                                sprintf(log_file, "index_logs/%s_delete_failure", time.curdate());
                                        }

                                        std::ofstream out(log_file, std::ios::app);
                                        out << time.curtime()  << "\t" << record.img_url << "\t" << imageid_sz << "\t" << timer.elapsed() << " sec"<< "\t" << reply.replycode() << std::endl;

				}

				return 1;
			};

			static int query_server(int query_port)
			{
				Socket sock;
				int    listenfd, connfd;

				if( (listenfd = sock.listenPort(query_port)) > 0 ){

					time_stamp();
					printf("======waiting for query client's request at port %d======\n", query_port);

					boost::threadpool::pool tp(30);

					while(1){
						if( (connfd = sock.acceptSocket(1)) == -1){
						usleep(2000);	
						continue;
						}

						tp.schedule(boost::bind(IndexServer::search, connfd));

					}
				}

				close(listenfd);
			};

			static int search(int connfd)
			{
				{
					boost::mutex::scoped_lock lock(thread_mutex);
					query_thread++;
				}

				bool success = false;
				bool matched = false;
				PidScore pidres;
				//std::string matchtype;

				time_stamp();
                                std::cout << "querying image " << std::endl;

				NiseReply reply;
				reply.replytype = REPLY_REDISSERVER_FAIL;

				signal(SIGPIPE, SIG_IGN);

				Record record;
				record.recvRecord(connfd);

				std::vector<std::vector<Key> > result;
				std::vector<Key> mergeresult;

				std::vector<KeyScore> keyresult;
				//std::vector<PidScore> pidresult;

				MultiIndex mindex;
				
				nise::Timer timer;

				if(mindex.redisConnect())
                                {
				//fingerprint check
				//int fp_num = mindex.queryFingerPrint(record.fingerprint, mergeresult);
				//int fp_num = 0;

				if(/*fp_num == 0 &&*/ record.desc.size() > 0)
				{	
//					result.resize(record.desc.size());

//					std::vector<boost::thread *> thread_pt;
//					thread_pt.resize(record.desc.size());

					int hamthres = 7;
					bool flip = true;

					mergeresult.clear();
					mindex.searchRecord(record, mergeresult,hamthres, flip);

/*
					//search Point in parrallel
					for(int i=0;i < thread_pt.size(); i++)
					{
						Point pt;
						pt.desc = record.desc[i];

						thread_pt[i] = NULL;
						thread_pt[i] = (boost::thread *)new boost::thread(boost::bind(IndexServer::searchPoint, pt, &result[i], hamthres, flip));
				
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

					for(int i=0;i < result.size();i++)
					{
				//		printf("final result size %d\n", result[i].size());
						mergeresult.insert(mergeresult.end(), result[i].begin(), result[i].end());
					}
				//	printf("merge result size %d\n", mergeresult.size());

	*/
					mindex.rankSearchResult(mergeresult, keyresult);
				}

			//	time_stamp();
			//	printf("%d sketches searched.\n", record.desc.size());
			//	printf("%d key score found.\n", keyresult.size());
				mindex.rankKeyScore(keyresult, reply.pidscore);
			//	printf("%d pid score found.\n", reply.pidscore.size());
			
				reply.replytype = REPLY_OK;	

				success = true;

				if(reply.pidscore.size() > 0)
				{
					matched = true;
					pidres = reply.pidscore[0];
				}

				}

				printf("Time cost : %f sec\n", timer.elapsed());

				reply.sendReply(connfd);

				close(connfd);

				nise::Time time;
				char log_file[100];

				if(success)
				{
					if(matched)
						sprintf(log_file, "index_logs/%s_query_matched", time.curdate());
					else
						sprintf(log_file, "index_logs/%s_query_unmatched", time.curdate());

				}
				else
				{
					sprintf(log_file, "index_logs/%s_query_failure", time.curdate());
				}

				std::ofstream out(log_file, std::ios::app);
				if(matched)
					out << time.curtime() << "\t" <<record.img_url << "\t" << record.fingerprint << "\t" << record.desc.size() << "\t"<< pidres.pid << "\t" << pidres.score <<"\t" << timer.elapsed() << " sec" << "\t" << reply.replycode() << std::endl;
				else
					out << time.curtime() << "\t" << record.img_url << "\t" << record.fingerprint << "\t" << record.desc.size() << "\t" << timer.elapsed() << " sec" << "\t" << reply.replycode() << std::endl;

				out.close();

				{
					boost::mutex::scoped_lock lock(thread_mutex);
					query_thread--;
					//query_success++;
                                        if(success)
                                        {
						query_success++;
					}
					else
                                        {
                                                query_failure++;
                                        }
				}

				return 1;
			};
			
			static int searchPoint(Point pt, std::vector<Key> *result, int hamthres = 7, bool flip = true)
			{
				
				MultiIndex mindex;
				
				if(mindex.redisConnect())
				{
					mindex.searchPoint(pt, *result, hamthres, flip);
				}
				else
					return -1;
				
			//	printf("search result size %d\n", result->size());
				return result->size();
			};

			static boost::mutex thread_mutex;
			static int insert_thread;
			static int delete_thread;
			static int query_thread;
			static int insert_success;
			static int delete_success;
			static int query_success;
			static int insert_failure;
			static int delete_failure;
			static int query_failure;
	};
};
#endif
