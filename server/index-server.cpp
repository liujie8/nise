/***************************************************************
# multi index server based on redis database
***************************************************************/

#include "index-server.h"

boost::mutex nise::IndexServer::thread_mutex;

int nise::IndexServer::insert_thread = 0;
int nise::IndexServer::delete_thread = 0;
int nise::IndexServer::query_thread = 0;

int nise::IndexServer::insert_success = 0;
int nise::IndexServer::delete_success = 0;
int nise::IndexServer::query_success = 0;

int nise::IndexServer::insert_failure = 0;
int nise::IndexServer::delete_failure = 0;
int nise::IndexServer::query_failure = 0;

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	IndexServer indexserv;

	boost::thread *query = indexserv.start_query_server();
	boost::thread *insert = indexserv.start_insert_server();
	boost::thread *del = indexserv.start_delete_server();
	boost::thread *control = indexserv.start_control_server();

	query->join();
	insert->join();
	del->join();
	control->join();

	return 1;
};
