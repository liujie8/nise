#ifndef _COMMON_FEATUREMAP
#define _COMMON_FEATUREMAP

#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "record.h"
#include "hamming.h"

namespace nise
{
	class FeatureMap
	{
		std::vector<std::multimap<Point, bool, PointComparator>> feat_maps;

		std::vector<unsigned> offset;
		std::vector<unsigned> query_keysize;
		std::vector<unsigned> insert_keysize;

		boost::mutex io_mutex;
		boost::mutex insert_mutex;

		FeatureMap()
		{
			int size = 4;
			int off = 0;
			feat_maps.resize(size);
		
			for(int i=0;i < 4;i++)
			{
				offset.push_back(off);
				off += 32;

				query_keysize.push_back(32);
				insert_keysize.push_back(128);
			}

		};
		
		~FeatureMap()
		{};

		bool save(std::ofstream &os, int idx)
		{
			std::multimap<Point, bool, PointComparator>::iterator iter;
			
			boost::scoped_lock lock(io_mutex);

			for(iter = feat_maps[idx].begin(); iter != feat_maps[idx].end(); iter++)	
			{
				iter->first.writePoint(os);
			};
		};

		bool merge(std::ofstream &os, std::ifstream &is, int idx)
		{

		};

		bool insert(Point &pt)
		{
			boost::mutex::scoped_lock lock(insert_lock);

			for(int i=0; i < feat_map.size(); i++)
			{
				feat_maps[i].insert(make_pair(pt, true));
			};

			return true;
		};
		
		bool search(Point &pt, std::vector<Key> &result)
		{
			Hamming hamming;
			std::multimap<Point, bool, PointComparator>::iterator lower_bound, upper_bound, iter;

			boost::mutex::scoped_lock lock(insert_lock);

			for(int i=0; i < feat_map.size(); i++)
			{
				lower_bound = feat_maps[i].lower_bound(pt);
				upper_bound = feat_maps[i].upper_bound(pt);
				
				for(iter = lower_bound; iter != upper_bound; iter++)
				{
					if(hamming(pt.desc, iter->first.desc) <= 3)
						result.push_back(iter->first.key);
				}
			};

			return true;
		};
	};
};

#endif
