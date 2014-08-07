/***************************************************************
# image feature extractor server
***************************************************************/

#include "extract-server.h"

boost::mutex nise::ExtractServer::thread_mutex;

int nise::ExtractServer::extract_thread = 0;
int nise::ExtractServer::extract_success = 0;
int nise::ExtractServer::extract_failure = 0;

int nise::ExtractServer::search_thread = 0;
int nise::ExtractServer::search_success = 0;
int nise::ExtractServer::search_failure = 0;

std::string nise::ExtractServer::img_path = "/data0/weibo_image";

std::string nise::ExtractServer::index_server_ip = "127.0.0.1";
int nise::ExtractServer::delete_port = 11111;
int nise::ExtractServer::insert_port = 9999;
int  nise::ExtractServer::query_port = 8888;

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	ExtractServer extractserv;

	boost::thread *extract = extractserv.start_extract_server();
	boost::thread *search = extractserv.start_search_server();
	boost::thread *control = extractserv.start_control_server();

	extract->join();
	search->join();
	control->join();

	return 1;
};
