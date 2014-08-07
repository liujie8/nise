#ifndef _COMMON_KEY
#define _COMMON_KEY

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <queue>

#include <boost/thread/mutex.hpp>

namespace nise
{
	typedef unsigned Key;

	class KeyDistributor
	{
		Key key;
		boost::mutex key_mutex;		
	public:
		KeyDistributor(Key k = 0)
		{
			key = k;
		};
		
		~KeyDistributor(){};

		Key getKey()
		{
			boost::mutex::scoped_lock lock(key_mutex);
                        return key;
		};	

		void setKey(Key k)
		{
			boost::mutex::scoped_lock lock(key_mutex); 
			key = k;
		};

		void updateKey(Key k)
		{
			boost::mutex::scoped_lock lock(key_mutex);

			if(key > k)
				key -= k;
			else
				key = 0;	
		};
		
		Key distributeKey()
		{
			boost::mutex::scoped_lock lock(key_mutex); 
			return key++;
		};
	};
/*
	class Keys
	{
	public:
		unsigned long long fingerprint;
		std::queue<Key> keys;
	public:
		Keys(){};
		~Keys(){};

		unsigned size()
		{
			return keys.size();
		};

		void printKeys()
		{
			std::cout << "FingerPrint : " << fingerprint << std::endl;
			std::cout << "Size : " << keys.size() << std::endl;
			for(int i=0;i < keys.size();i++)
			{
				std::cout << keys[i] << "\t";
			}
			std::cout << std::endl;
		};

		bool readKeys(std::ifstream &is)
		{
			keys.clear();

			unsigned keys_size = 0;

			is.read((char *)&keys_size, sizeof(unsigned));

			keys.resize(keys_size);

			if(keys_size > 0)
			{
				is.read((char *)&fingerprint, sizeof(unsigned long long)); //fingerprint
				is.read((char *)&keys.at(0), keys_size * sizeof(Key)); //keys set
			}

			if(is)
				return true;
			else
				return false;	
		};

		bool writeKeys(std::ofstream &os)
		{
			unsigned keys_size = keys.size();

			if(keys_size > 0)
			{
				os.write((char *)&keys_size, sizeof(unsigned)); //keys size
				os.write((char *)&fingerprint, sizeof(unsigned long long)); //fingerprint
				os.write((char *)&keys.at(0), keys_size * sizeof(Key)); //keys set
			}

			if(os)
				return true;
			else
				return false;
		};
	};

	class KeyKeys
	{
		std::map<Key, Keys> key_keys;  //key -> Keys
		std::map<unsigned long long, Keys> fp_keys; //fingerprint -> Keys
	public:
		KeyKeys(){};
		~KeyKeys(){};

		int load(std::ifstream &is)
		{
			Keys keys;

			key_keys.clear();

			int cnt = 0;

			while(keys.readKeys(is))
			{
				cnt++;

				if(keys.size())
					key_keys.insert(make_pair(keys.keys[0], keys));
			};

			return cnt;
		};

		int save(std::ofstream &os)
		{
			Keys keys;

			int cnt = 0;

			std::map<Key, Keys>::iterator iter;

			for(iter = key_keys.begin(); iter != key_keys.end(); iter++)
			{
				iter->writeKeys(os);

				if(os)
					cnt++;
				else
					break;
			}

			return cnt;
		};

		bool find(Key &key, Keys &keys)
		{
			std::map<Key, Keys>::iterator iter;

			iter = key_keys.find(key);

			if(iter != key_keys.end())
			{
				keys = iter->second;
				return true;
			}
			return false;
		};

		bool map(std::vector<Key> &keys)
		{
			std::vector<Key> tmp;
			tmp.swap(keys);

			Keys k;

			for(int i=0;i < tmp.size();i++)
			{
				if(find(tmp[i], k))
				{
					for(int j=0;j<k.size();j++)
						keys.push_back(k.keys[j]);
				}
			}
			return true;
		};

		int update(Key up_key)
		{
			std::map<Key, Keys>::iterator iter;

			int cnt = 0;

			for(iter = key_keys.begin(); iter != key_keys.end(); iter++)
			{
				Keys &k = iter->second;

				for(int i = 0; i<k.size();i++) 
					if(k.keys[i] < up_key)
					{
						cnt++;
						k.keys.erase(&k.keys[i]);
					}
					else
						k.keys[i] -= up_key;
			}

			return cnt;	
		};
	};
*/
	class KeyPair
	{
		public:
			Key key;
			std::string pid;

		public:

			KeyPair(){};
			~KeyPair(){};

		void printPair()
		{
			std::cout << key << "\t" << pid << std::endl;
		};

		bool readPair(std::ifstream &is)
		{
			is.read((char *)&key, sizeof(Key));

			int pid_size = 0;
			
			is.read((char *)&pid_size, sizeof(int));

			pid.resize(pid_size);

			if(pid_size > 0)
				is.read((char *)&pid.at(0), pid_size);

			if(is)
				return true;
			else
				return false;
		};

		bool writePair(std::ofstream &os)
		{
			os.write((char *)&key, sizeof(Key));

			int pid_size = pid.size();

			os.write((char *)&pid_size, sizeof(int));

			if(pid_size > 0)
				os.write((char *)&pid.at(0), pid_size);

			if(os)
				return true;
			else
				return false;
		};
	};

	class KeyVec
	{
		std::vector<KeyPair> key_vec;
		unsigned init_key;

		boost::mutex vec_mutex;
	public:
		KeyVec()
		{
			init_key = 0;
		};
		~KeyVec(){};

		int load(std::ifstream &is)
		{
			KeyPair key_pair;

			key_vec.clear();

			 int cnt = 0;

                        while(key_pair.readPair(is))
                        {
                                cnt++;
                                key_vec.push_back(key_pair);

				if(cnt == 1)
					init_key = key_pair.key;
                        };

                        return cnt;
		};

		int append(std::ifstream &is)
		{
			KeyPair key_pair;

			int cnt = 0;

			while(key_pair.readPair(is))
			{
				cnt++;
				key_vec.push_back(key_pair);
			};

			return cnt;
		};

		int save(std::ofstream &os, Key addkey = 0)
		{
			KeyPair key_pair;

			int cnt = 0;

			std::vector<KeyPair>::iterator iter;

			for(iter = key_vec.begin(); iter != key_vec.end(); iter++)
			{
				key_pair.key = iter->key + addkey;
				key_pair.pid = iter->pid;

				key_pair.writePair(os);
			
	//			iter->writePair(os);
				if(os)
					cnt++;
				else
					break;
			}

			return cnt;
		};

		void clear()
		{
			key_vec.clear();
		};

		bool push(Key &key, std::string &pid)
		{
			KeyPair keypair;
			keypair.key = key;
			keypair.pid = pid;

			key_vec.push_back(keypair);

			return true;
		};

		bool find(Key &key, std::string &pid)
		{
			unsigned int idx = key - init_key;

			if(idx < key_vec.size())
			{
				pid = key_vec[idx].pid;
				return true;
			}
			else
				return false;
		};

		bool map(std::vector<Key> &keys, std::vector<std::string> &pids)
		{
		//	pids.clear();
			std::string pid;

			for(int i=0;i < keys.size();i++)
			{
				if(find(keys[i], pid))
					pids.push_back(pid);
			}
			return true;
		};
	};

	class KeyMap
	{
		std::map<Key, std::string> key_map;
	public:
		KeyMap(){};
		~KeyMap(){};
/*
		std::map<Key, std::string>& getMap()
		{
			return key_map;
		};
*/
		int load(std::ifstream &is)
		{
			KeyPair key_pair;

			key_map.clear();

			int cnt = 0;

			while(key_pair.readPair(is))
			{
				cnt++;
				key_map.insert(make_pair(key_pair.key, key_pair.pid));
			};
		
			return cnt;
		};
		
		int save(std::ofstream &os, Key addkey = 0)
		{
			KeyPair key_pair;

			int cnt = 0;
		
			std::map<Key, std::string>::iterator iter;

			for(iter = key_map.begin(); iter != key_map.end(); iter++)
			{
				key_pair.key = iter->first + addkey;
				key_pair.pid = iter->second;

				key_pair.writePair(os);
				
				if(os)
					cnt++;
				else
					break;
			}

			return cnt;
		};

		bool find(Key &key, std::string &pid)
		{
			std::map<Key, std::string>::iterator iter;
			
			iter = key_map.find(key);

			if(iter != key_map.end())
			{
				pid = iter->second;
				return true;
			}
			return false;
		};
	
		bool map(std::vector<Key> &keys, std::vector<std::string> &pids)
		{
			pids.clear();
			
			std::string pid;

			for(int i=0;i < keys.size();i++)
			{
				if(find(keys[i], pid))
					pids.push_back(pid);
			}
			return true;
		};

		int update(std::ofstream &os, Key up_key)
		{
                        std::map<Key, std::string>::iterator iter;

			KeyPair key_pair;
			int cnt = 0;

                        for(iter = key_map.begin(); iter != key_map.end(); iter++)
                        {
                                key_pair.key = iter->first;
				key_pair.pid = iter->second;

                                if(key_pair.key >= up_key)
                                {
					key_pair.key -= up_key;
					key_pair.writePair(os);
				}
                                else
                                        cnt++;
                        }
			
			return cnt;
		};
	};

};

#endif
