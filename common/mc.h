#ifndef _COMMON_MEMCACHE
#define _COMMON_MEMCACHE
//memcache client
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>

#include <libmemcached/memcached.h>

namespace nise
{
	class McClient
	{
		public:
			McClient()
			{
				memc = NULL;
				servers = NULL;
			};

			~McClient()
			{
				//free
				memcached_free(memc);
				memc = NULL;
			};

			bool getSrvs()
			{
				
			};

			bool addSrv(char* ip, int port)
			{
				memc = memcached_create(NULL);	
				ervers = memcached_server_list_append(NULL, ip, port, &rc);
				rc = memcached_server_push(memc, servers);
				memcached_server_free(servers);
				memcached_behavior_set(m_pMemc,MEMCACHED_BEHAVIOR_BINARY_PROTOCOL,0);
			};

			bool deleteSrv(char* ip, int port)
			{
				
			};

			bool getItem(char* szKey,int iKeyLen,char* szValue,int iValueLen)
			{
				szValue = memcached_get(memc,szKey,(size_t)iKeyLen,(size_t*)&iValueLen,(uint32_t*)&flags,&rc);
			
				if(rc == MEMCACHED_SUCCESS)
				{
					return true;
				}
				fprintf(stderr,"%s\n",memcached_strerror(memc,rc));
				return false;
			};

			bool setItem(char* szKey,int iKeyLen,char* szValue,int iValueLen)
			{
				rc = memcached_set(memc,szKey,iKeyLen,szValue,iValueLen, (time_t)0, 0);

				if(rc == MEMCACHED_SUCCESS)
				{
					return true;
				}
				fprintf(stderr,"%s\n",memcached_strerror(memc,rc));
				return false;
			};

			bool deleteItem(char* szKey,int iKeyLen)
			{
				rc = memcached_delete(memc,szKey,iKeyLen,0);

				if(rc == MEMCACHED_SUCCESS)
				{
					return true;
				}
				fprintf(stderr,"%s\n",memcached_strerror(memc,rc));
				return false;
			};

			bool clear()
			{};

		private:
			memcached_st * memc;
			memcached_return rc;
			memcached_server_st * servers;
			uint32_t flags;
	};
};

#endif
