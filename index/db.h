//database
#ifndef _DB
#define _DB

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>

#include "../common/key.h"
#include "../common/record.h"
#include "../common/hamming.h"
#include "../common/eval.h"
#include "../common/log.h"

#define SEP "/"

namespace nise
{
	struct Range
	{
		unsigned offset;
		unsigned length;
		
		Range(unsigned off =0, unsigned len =0)
		{
			offset = off;
			length = len;
		};

		Range operator * (unsigned int sample_rate)
		{
			Range rng;
			rng.offset = offset * sample_rate;
			rng.length = length * sample_rate;
			
			return rng;
		};
	};

	class SampleIndex
	{
		std::vector<Point> sample;
		PointComparator cmp;

	public:		
		SampleIndex(unsigned int offset = 0, unsigned int key_size = 32)
		{
			cmp.setOffset(offset);
			cmp.setKeysize(key_size);
		};
		
		void setKeysize(unsigned int key_size)
		{
                        cmp.setKeysize(key_size);
		};
		
		int getKeysize()
		{
			return cmp.getKeysize();
		};
		
		int getOffset()
		{
			return cmp.getOffset();
		};
		
		int load(std::ifstream &is)
		{
			sample.clear();
			
			Point pt;
			
			while(pt.readPoint(is, true))
			{
				sample.push_back(pt);
			};
			
			return sample.size();
		};

		int save(std::ofstream &os)
		{
			int cnt = 0;
			for(int i=0; i< sample.size(); i++)
			{
				if(sample[i].writePoint(os, true))
					cnt++;	
			};
			
			return cnt;
		};
		
		bool search(Point &pt, Range &range)
		{
			std::vector<Point>::iterator lower, upper;

			lower = lower_bound(sample.begin(), sample.end(), pt, cmp);
			upper = upper_bound(sample.begin(), sample.end(), pt, cmp);
			
			if(lower != sample.begin())
				lower -= 1;
			range.offset = (unsigned int)(lower - sample.begin());
			range.length = (unsigned int)(upper - lower);

			return true;
		};
	};

	class Scanner
	{
		std::vector<Point> points;
		PointComparator cmp;
		Range rng;
	public:
		Scanner(unsigned int offset = 0, unsigned int key_size = 32)
		{
			cmp.setOffset(offset);
                        cmp.setKeysize(key_size);
		};
		~Scanner()
		{};
		
		PointComparator & getCmparator()
		{
			return cmp;
		};

		Range& getRange()
		{
			return rng;
		};

		void printPoints()
		{
			for(int i=0;i < points.size(); i++)
				points[i].printPoint();
		};

                int load(std::ifstream &is, Range &range)
                {
                        points.clear();
			
			is.seekg(range.offset * Point::size(), std::ios::beg);
			
			points.resize(range.length);

			if(points.size())
				is.read(reinterpret_cast<char *>(&points.at(0)), range.length * sizeof(points.at(0)));

			int real_size = is.gcount()/sizeof(Point);
			points.resize(real_size);

			rng.offset = 0;
			rng.length = points.size();
			
                        return points.size();
                };

		void locate_range(Point &query)
		{
			std::vector<Point>::iterator lower, upper;

			lower = lower_bound(points.begin(), points.end(), query, cmp);
			upper = upper_bound(points.begin(), points.end(), query, cmp);

			rng.offset = (unsigned int)(lower - points.begin());
			rng.length = (unsigned int)(upper - lower);
		};

		void scan(Point &query, int dist, std::vector<Key> &result, bool lock = false, bool relocate = true)
		{
			if(relocate)
				locate_range(query);
			
			Hamming hamming;

			for(int i = rng.offset; i < rng.offset + rng.length; i++)
			{
//				int d = hamming((Chunk *)query.desc.sketch, (Chunk *)points[i].desc.sketch);
			
				int d = hamming.popcount((unsigned long long *)query.desc.sketch, (unsigned long long *)points[i].desc.sketch);
//				std::cout << "distance: " << d << std::endl;
				if(d <= dist)
					result.push_back(points[i].key);
			}		
		};
	
	};

	class DB
	{
		public:
			DB(const char * db_config)
			{
			//	db_out.open("db_out");
				log = NULL;
				key_vec = NULL;

				std::ifstream is(db_config);

				if(is)
				{
					unsigned offset, key_size;
					std::string base_dir, index_path, sample_path;

					is >> sample_rate;
					is >> base_dir;
					is >> conf_path;
					conf_path = base_dir + SEP + conf_path;

					minkey = maxkey = indexsize = 0;

					std::ifstream is_conf(conf_path.c_str());

					if(is_conf)
					{
						is_conf >> minkey >> maxkey >> indexsize;
					}
					else
						std::cout << "Failed to load conf file " << conf_path << std::endl;

					std::cout << "MinKey : " << minkey << "\tMaxKey : " << maxkey << "\tIndexSize : " << indexsize << std::endl;
					is_conf.close();

					while(is >> offset >> key_size >> index_path >> sample_path)
					{	
						SampleIndex * sample = new SampleIndex(offset, key_size);
						
                                                index_path = base_dir + SEP + index_path;
						sample_path = base_dir + SEP + sample_path;

						std::ifstream is2(sample_path.c_str(), std::ios::binary);

						if(is2)
						{
							int cnt = sample->load(is2);
							std::cout << sample_path << " : " << cnt << " sample points loaded." << std::endl;
						}
						is2.close();

						samples.push_back(sample);
						sample_paths.push_back(sample_path);
						index_paths.push_back(index_path);
					}
					db_ok = true;
					std::cout << samples.size() << " sample files loaded. " << std::endl;
				}
				else
				{
					std::cout << "Failed to open " << db_config << std::endl;
					db_ok = false;
				}

				is.close();
			};

			~DB()
			{
		//		db_out.close();

				for(int i=0;i < samples.size();i++)
				{
					delete samples[i];
				}

			};

			bool reload()
			{
			//	if(db_ok)db_ok = false;	
			//	else
			//		return false;
				
				db_ok = false;

				minkey = maxkey = indexsize = 0;

				std::ifstream is_conf(conf_path.c_str());

				if(is_conf)
				{
					is_conf >> minkey >> maxkey >> indexsize;
				}
				else
				{
					std::cout << "Failed to load conf file " << conf_path << std::endl;
					return false;
				}

				std::cout << "MinKey : " << minkey << "\tMaxKey : " << maxkey << "\tIndexSize : " << indexsize << std::endl;
				is_conf.close();

				for(int i=0;i<sample_paths.size();i++)
				{
					std::ifstream is(sample_paths[i].c_str(), std::ios::binary);

					if(is)
					{
						int cnt = samples[i]->load(is);
						std::cout << sample_paths[i] << " : " << cnt << " sample points loaded." << std::endl;
					}
					else
						return false;
					is.close();
				}
			
				db_ok = true;
				return db_ok;
			};

			void clearSample()
			{
                                for(int i=0;i < samples.size();i++)
                                {
					if(samples[i])
					{
                                       		delete samples[i];
						samples[i] = NULL;
					}
                                }

				samples.clear();
			};

			bool dbOk()
			{
				return db_ok;
			};

			void setLog(Log * db_log = NULL)
			{
				log = db_log;
			};

			void setKeyVec(KeyVec * db_keyvec = NULL)
			{
				key_vec = db_keyvec;
			};

			unsigned int getSampleRate()
			{
				return sample_rate;
			};

			std::string getConfPath()
			{
				return conf_path;
			};

			unsigned getMinKey()
			{
				return minkey;
			};

			unsigned getMaxKey()
			{
				return maxkey;
			};

			unsigned long long getIndexSize()
			{
				return indexsize;
			};

			std::vector<std::string> getIndexPath()
			{
				return index_paths;
			};

			std::vector<std::string> getSamplePath()
			{
				return sample_paths;
			};

			int searchPoint(Point &query, std::vector<Key> &result)
			{
				std::cout << "Searching Point ..." << std::endl;

				 for(int i=0; i < samples.size();i++)
                                {
                                        Range range;

                                        unsigned offset = samples[i]->getOffset();
                                        unsigned key_size = samples[i]->getKeysize();

                                        Scanner scanner(offset, key_size);

                                        std::ifstream is(index_paths[i].c_str(), std::ios::binary);

					if(is)
					{
						samples[i]->search(query, range);
						range = range * sample_rate;
						scanner.load(is, range);
						
					//	range.offset = 0;
					//	range.length = 300000;
					//	scanner.load(is, range);
						scanner.scan(query, 3, result);
                                        }
                                        else
                                                std::cout << "Failed to open file " << index_paths[i] << std::endl;

                                        is.close();
                                }

				 std::sort(result.begin(), result.end());           
				 result.resize(std::unique(result.begin(), result.end()) - result.begin());

				 return result.size();
			};

			int search(Record &record, std::vector<Key> &result)
			{
				std::cout << "Searching Record ..." << std::endl;

				if(log)log->log("DB::searchRecord started.");

				for(int i=0; i < samples.size();i++)
				{
					Range range;
					Point pt;

					unsigned offset = samples[i]->getOffset();
					unsigned key_size = samples[i]->getKeysize();

					Scanner scanner(offset, key_size);

					std::ifstream is(index_paths[i].c_str(), std::ios::binary);
					
					Timer timer;
					float time1=0,time2=0,time3=0;

					if(is)
					{
						for(int j=0; j < record.desc.size(); j++)
						{
							pt.desc = record.desc[j];
							
							timer.restart();

							samples[i]->search(pt, range);

							time1 += timer.elapsed();
							timer.restart();
							range = range * sample_rate;
							scanner.load(is, range);
	
							time2 += timer.elapsed();
                                                        timer.restart();

							scanner.scan(pt, 3, result);
							
							time3 += timer.elapsed();
						}

			//			db_out << time1 << "  " << time2 << "  " << time3 << " " << std::endl;
						std::cout << "binary search :" << time1 << " seconds."<< std::endl;
						std::cout << "load :" << time2 << " seconds." << std::endl;
						std::cout << "scan :" << time3 << " seconds." << std::endl;
					}
					else
						std::cout << "Failed to open file " << index_paths[i] << std::endl;

					is.close();
				}
				
			//	db_out << std::endl;

				std::sort(result.begin(), result.end());           
				result.resize(std::unique(result.begin(), result.end()) - result.begin());

				if(log)log->log("DB::searchRecord finished.");

				return result.size();
			};

			bool map(std::vector<Key> &keys, std::vector<std::string> &pids)
			{
				if(!key_vec)return false;
				
				key_vec.map(keys, pids);
				return true;
			};
		private:
			std::vector<SampleIndex *> samples;
			std::vector<std::string> index_paths;
			std::vector<std::string> sample_paths;
			std::string conf_path;
			unsigned int minkey, maxkey;
			unsigned long long indexsize;
			unsigned int sample_rate;
			bool db_ok;
			
			KeyVec * key_vec;
			Log * log;
//			std::ofstream db_out;
	};

};

#endif
