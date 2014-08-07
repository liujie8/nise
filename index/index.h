#ifndef _INDEX
#define _INDEX

#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "../common/key.h"
#include "../common/record.h"
#include "../common/hamming.h"
#include "../common/log.h"

namespace nise
{
	class Index
	{
		struct PointComp
		{
			static unsigned int offset;
			static unsigned int keysize;

			PointComp(unsigned int off = 0, unsigned int size = 128)
			{
				offset = off;
				keysize = size;
			};
			~PointComp(){};

			bool operator() (const Point &p1, const Point &p2)
			{
				Chunk * c1 = (Chunk *) p1.desc.sketch;
				Chunk * c2 = (Chunk *) p2.desc.sketch;

				for(int i=0;i < keysize/CHUNK_BIT; i++)
				{
					int j = (i + offset/CHUNK_BIT) % DATA_CHUNK;
					if(c1[j] > c2[j]) return false;
					if(c1[j] < c2[j]) return true;
				}
				return false;
			};
		};

		Log *log;  //log

		std::multimap<Point, bool, PointComp> index[2][4];
		KeyDistributor key_dis[2];
		KeyVec key_vec[2];
//		KeyMap key_map[2];
		int current;
		int prev;

		std::vector<unsigned> offset;
		std::vector<unsigned> query_keysize;
		std::vector<unsigned> insert_keysize;

		boost::mutex io_mutex;
		boost::mutex map_mutex;
public:
		Index()
		{
			log = NULL;

			current = 0;
			prev = 1;

			int size = 4;
			int off = 0;
		
			for(int i=0;i < size;i++)
			{
				offset.push_back(off);
				off += 32;

				query_keysize.push_back(32);
				insert_keysize.push_back(128);
			}
			
			for(int i=0;i<2;i++)
			{
				reset(i);
			}
		};
		
		~Index()
		{};

		void setLog(Log * index_log = NULL)
		{
			log = index_log;
		};

		void reset(int idx)
		{
			boost::mutex::scoped_lock lock1(io_mutex);
                        boost::mutex::scoped_lock lock2(map_mutex);

			key_vec[idx].clear();
			key_dis[idx].setKey(0);
	
			for(int i=0;i<4;i++)
				index[idx][i].clear();

			if(log)log->log("Index::reset");
		//	index[idx].resize(4);
		};

		void switchIndex()
		{
			boost::mutex::scoped_lock lock1(io_mutex);
			boost::mutex::scoped_lock lock2(map_mutex);

			if(current == 1)
			{
				current = 0;
				prev = 1;
			}
			else
			{
				current = 1;
				prev = 0;
			}

			key_vec[current].clear();
			key_dis[current].setKey(0);

			for(int i=0;i<4;i++)
				index[current][i].clear();

			if(log)log->log("Index::switchIndex");
//			index[current].resize(4);

			//reset(current);
		};

		int curIndex()
		{
			return current;
		};

		int prevIndex()
		{
			return prev;
		};

		unsigned getKeysize(int idx)
		{
			if(idx < 2 && idx >= 0)
				return key_dis[idx].getKey();
			else
				return 0;
		};

		unsigned getMapsize(int idx)
		{
			if(idx < 2 && idx >= 0)
                                return index[idx][0].size();
                        else
                                return 0;
		};

		int load(std::ifstream &is, int idx)
		{
			std::multimap<Point, bool, PointComp>::iterator iter;
		};

		bool saveIndex(std::ofstream &os, int idx, Key addkey = 0)
		{
			if(idx >= 4)return false;

			std::multimap<Point, bool, PointComp>::iterator iter;
			
			boost::mutex::scoped_lock lock(io_mutex);
			
			if(log)log->log("Index::saveIndex started.");

			Point pt;

			for(iter = index[prev][idx].begin(); iter != index[prev][idx].end(); iter++)	
			{
				pt.key = iter->first.key + addkey;
				pt.desc = iter->first.desc;
				pt.writePoint(os, true);

			//	iter->first.writePoint(os, true);
			};

			if(log)log->log("Index::saveIndex finished.");

			return true;
		};
		
		bool saveKeyVec(std::ofstream &os, Key addkey = 0)
		{
			boost::mutex::scoped_lock lock(io_mutex);
			
			if(log)log->log("Index::saveKeyVec started.");

			key_vec[prev].save(os, addkey);

			if(log)log->log("Index::saveKeyVec finished.");

			return true;
		};
/*
		bool merge(std::ofstream &os, std::ifstream &is, int idx)
		{
			boost::mutex::scoped_lock lock(io_mutex);
			return true;
		};

		bool update(Key key)
		{
			{
				boost::mutex::scoped_lock lock(map_lock);
				key_dis.updateKey(key);
				key_map.update(key);
			}

			boost::mutex::scoped_lock lock(io_mutex);
			updateIndex();
			
		};
*/
		bool insertPoint(Point &pt)
		{
			boost::mutex::scoped_lock lock(map_mutex);

			for(int i=0; i < 4; i++)
			{
				PointComp::offset = offset[i];
				PointComp::keysize = insert_keysize[i];

				index[current][i].insert(std::make_pair(pt, true));
			};

			return true;
		};

		Key insertRecord(Record &record)
		{
			Point pt;

			boost::mutex::scoped_lock lock(map_mutex);

			pt.key = key_dis[current].distributeKey();
			key_vec[current].push(pt.key, record.img_url);

			if(log)log->log("Index::insertRecord started.");

			for(int i=0; i < record.desc.size();i++)
			{
				pt.desc = record.desc[i];
				//	insertPoint(pt);

				for(int j=0; j < 4; j++)
				{
					PointComp::offset = offset[j];
					PointComp::keysize = insert_keysize[j];

					index[current][j].insert(std::make_pair(pt, true));
				};
			}

			if(log)log->log("Index::insertRecord finished.");

			return pt.key;
		};
		
		bool searchPoint(Point &pt, std::vector<Key> result[2])
		{
			Hamming hamming;
			std::multimap<Point, bool, PointComp>::iterator lower_bound, upper_bound, iter;

			boost::mutex::scoped_lock lock(map_mutex);

			for(int i=0; i < 2;i++)
				for(int j=0; j < 4; j++)
				{
					PointComp::offset = offset[j];
					PointComp::keysize = query_keysize[j];

					lower_bound = index[i][j].lower_bound(pt);
					upper_bound = index[i][j].upper_bound(pt);

					for(iter = lower_bound; iter != upper_bound; iter++)
					{
						if(hamming(pt.desc.sketch, iter->first.desc.sketch) <= 3)
							result[i].push_back(iter->first.key);
					}
				};

			for(int i=0;i<2;i++)
			{
				std::sort(result[i].begin(), result[i].end());
				result[i].resize(std::unique(result[i].begin(), result[i].end()) - result[i].begin());
			}
			return true;
		};

		bool searchRecord(Record &record, std::vector<Key> result[2])
		{
			Point pt;

			Hamming hamming;
			std::multimap<Point, bool, PointComp>::iterator lower_bound, upper_bound, iter;

			boost::mutex::scoped_lock lock(map_mutex);

			if(log)log->log("Index::searchRecord started.");

			for(int i=0;i < record.desc.size();i++)
			{
				pt.desc = record.desc[i];
				//		searchPoint(pt, result);
				for(int j=0; j < 2;j++)
					for(int k=0; k < 4; k++)
					{
						PointComp::offset = offset[k];
						PointComp::keysize = query_keysize[k];

						lower_bound = index[j][k].lower_bound(pt);
						upper_bound = index[j][k].upper_bound(pt);

						for(iter = lower_bound; iter != upper_bound; iter++)
						{
							if(hamming(pt.desc.sketch, iter->first.desc.sketch) <= 3)
								result[j].push_back(iter->first.key);
						}
					};
			}

			for(int i=0;i< 2 ;i++)
			{
				std::sort(result[i].begin(), result[i].end());
				result[i].resize(std::unique(result[i].begin(), result[i].end()) - result[i].begin());
			}

			if(log)log->log("Index::searchRecord finished.");
			return true;
		};

		bool map(std::vector<Key> keys[2], std::vector<std::string> &pids)
		{
			boost::mutex::scoped_lock lock(map_mutex);

			if(log)log->log("Index::map");

			for(int i=0;i<2;i++)
				key_vec[i].map(keys[i], pids);

			return true;
		};
	};
};

#endif
