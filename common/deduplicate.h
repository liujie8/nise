#ifndef _COMMON_DEDUPLICATE
#define _COMMON_DEDUPLICATE

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <queue>
                
#include <boost/thread/mutex.hpp>

#include "key.h"

namespace nise
{
	struct KeySet
	{
		std::queue<Key> keyset;

		void push(Key key)
		{
			keyset.push(key);
		};

		void pop()
		{
			keyset.pop();
		};

		Key top()
		{
			return keyset.front();
		};

		int size()
		{
			return keyset.size();
		};

		bool readKeySet(std::ifstream &is)
		{
			unsigned int key_sz = 0;

			is.read((char *)&key_sz, sizeof(unsigned int));

			keyset.resize(key_sz);

			if(key_sz > 0)
				is.read(reinterpret_cast<const char *>(&keyset[0]), sizeof(Key) * key_sz);

			if(is)return true;
			else
				return false;
		};
		
		bool writeKeySet(std::ofstream &os)
		{
			unsigned int key_sz = keyset.size();

                        os.write((char *)&key_sz, sizeof(unsigned int));

                        if(key_sz > 0)
                                os.write(reinterpret_cast<const char *>(&keyset[0]), sizeof(Key) * key_sz);

                        if(os)return true;
                        else
                                return false;
		};
	};

	struct KeyList
	{
		unsigned long long fingerprint;
		KeySet keyset;
	public:
		KeyList(){};
		~KeyList(){};

		bool readList(std::ifstream &is)
		{
			is.read((char *)&fingerprint, sizeof(unsigned long long));
			return keyset.readKeySet(is);
		};

		bool writeList(std::ofstream &os)
		{
			os.write((char *)&fingerprint, sizeof(unsigned long long));
                        return keyset.writeKeySet(os);
		};

		bool recvList(int connfd)
		{
			return true;
		};
	
		bool sendList(int connfd)
		{
			return true;
		};
	};

	class Deduplicator
	{
		std::map<unsigned long long, KeySet> fingerset;
		boost::mutex map_mutex;
	public:
		Deduplicator(){};
		~Deduplicator(){};

		bool checkFingerPrint(unsigned long long fingerprint, Key key)
		{
			std::map<unsigned long long, KeySet>::iterator iter;
			
			boost::mutex::scoped_lock lock(map_mutex);

			iter = fingerset.find(fingerprint);
			if(iter != fingerset.end())
			{
				iter->second.push(key);
				return true;
			}
			else
			{
				KeySet keyset;
				keyset.push(key);
				fingerset.insert(std::make_pair(fingerprint, keyset));
				return false;
			}
		};

		bool findKeyList(KeyList & keylist)
		{
			std::map<unsigned long long, KeySet>::iterator iter;

                        boost::mutex::scoped_lock lock(map_mutex);

                        iter = fingerset.find(fingerprint);

                        if(iter != fingerset.end())
                        {
				keylist.fingerprint = iter->first;
                                keylist.keyset = iter->second;
                                return true;
                        }
			else 
				return false;
		};

		int load(std::ifstream &is)
		{
			std::map<unsigned long long, KeySet>::iterator iter;

			boost::mutex::scoped_lock lock(map_mutex);

			fingerset.clear();

			int cnt = 0;

			KeyList keylist;			

			while(keylist.readList(is))
			{
				fingerset.insert(std::make_pair(keylist.fingerprint, keylist.keyset));
				cnt++;
			}
			
			return cnt;
		};		

		int save(std::ofstream &os)
		{
			std::map<unsigned long long, KeySet>::iterator iter;

			boost::mutex::scoped_lock lock(map_mutex);

			int cnt = 0;
			
			KeyList keylist;

			for(iter = fingerset.begin();iter != fingerset.end();iter++)
			{
			//	os << iter->first << "\t"<< iter->second <<std::endl;
				os.write((char *)&(iter->first), sizeof(unsigned long long));
			
				int key_sz = iter->second.size();

				os.write((char *)&key_sz, sizeof(int));
				
				if(key_sz > 0)
					os.write(reinterpret_cast<const char *>(&(iter->second.keyset[0])), sizeof(Key) * key_sz);
			
				if(os)cnt++;
			}
			return cnt;
		};				
	};
};

#endif
