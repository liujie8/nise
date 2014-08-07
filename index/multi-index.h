/******************************************************************************
#An implementation of binary code indexing algorithm for an image search engine
******************************************************************************/
#ifndef _MULTIINDEX
#define _MULTIINDEX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <set>
#include <vector>
#include <bitset>
#include <iostream>
//#include <fstream>
#include <string>

#include "../common/key.h"
#include "../common/record.h"
#include "../common/hamming.h"
#include "../common/log.h"
#include "../common/nisereply.h"

//multi indexing hash using redis 
#include "../include/hiredis.h"

/*******************************************************
# db allocation strategy
# db 0-3 sketch substring --> sketchid list (key-->list)
# db 4 sketchid --> sketch (key-->string)
# db 5 imageid --> image attribute (key-->hash)
# db 6 imagepid --> imageid list (key-->list)
# db 7 imagefingerprint --> imageid list (key-->list)
# db 8 global settings
*******************************************************/
			
namespace nise
{
	//global key allocator
	class KeyAllocator
	{
		redisContext *c;
	public:
		KeyAllocator()
		{
			c = NULL;
		};

		~KeyAllocator()
		{
			free();
		};

		void free()
		{
			if(c)redisFree(c);
			c = NULL;
		};

		//timoout in seconds
                bool redisConnect(char *ip = "127.0.0.1",int port = 6379,int  timeout_sec = 0,int timeout_usec = 100000)
                {
                        struct timeval t;
                        t.tv_sec = timeout_sec;
                        t.tv_usec = timeout_usec;

			free();

                        c = redisConnectWithTimeout(ip, port, t);
                        if (c == NULL || c->err) {
                                if (c) {
                                        printf("failed to connect to redis server %s %d , error type: %s\n", ip, port, c->errstr);
                                        free();
                                } else {
                                        printf("Connection error: can't allocate redis context\n");
                                }
                                return false;
                        }

                        return true;
                };

                bool redisSetDB(int idx)
                {
                        redisReply *reply;

                        reply = (redisReply *)redisCommand(c, "select %d", idx);

                        if(!reply)return false;

                        if(strcmp("OK", reply->str) == 0)
                        {
                                freeReplyObject(reply);
                                return true;
                        }
                        else
                        {
                                freeReplyObject(reply);
                                return false;
                        }
                };

		//allocate key from key list , otherwise using incr to get a key
		Key allocateKey()
		{
			Key key=0;

			if(redisSetDB(8))
			{
				redisReply *reply;
				reply = (redisReply *)redisCommand(c, "lpop keylist");
				if(!reply)return key;
	
				if(reply->type == REDIS_REPLY_STRING)
				{
					key = *((Key *)reply->str);
					freeReplyObject(reply);
				}
				else
				{
					freeReplyObject(reply);

					reply =  (redisReply *)redisCommand(c, "incr curkey");
					if(!reply)return key;

					if(reply->type == REDIS_REPLY_INTEGER)
					{
						key = reply->integer;
					}
					freeReplyObject(reply);
				}
			}
		
			return key;
		};

		//put a key back to key list
		bool recycleKey(Key key)
		{
			if(redisSetDB(8))
			{
				redisReply *reply;
				reply = (redisReply *)redisCommand(c, "rpush keylist %b", (char *)&key, sizeof(Key));
				if(!reply)return false;

				if(reply->type == REDIS_REPLY_INTEGER && reply->integer > 0)
				{
					freeReplyObject(reply);
					return true;
				}
				else
				{
					freeReplyObject(reply);
					return false;
				}
			}
			else
				return false;
		};	
	};
/*
	//for result rank
	struct KeyScore
	{
		Key key;
		float score;

		KeyScore()
		{
			reset();	
		};

		~KeyScore(){};

		void reset()
		{
			key = 0;
			score = 0;
		};
	};
*/
	bool cmpKeyScore(const KeyScore &ks1, const KeyScore &ks2)
	{
		return ks1.score > ks2.score;
	};
/*
	struct PidScore
	{
		std::string pid;
		float score;
	
		PidScore()
		{
			reset();
		};

		~PidScore(){};
		
		void reset()
		{
		pid = "";
		score = 0;
		};
		};
 */
	bool cmpPidScore(const PidScore &ps1, const PidScore &ps2)
	{
		return ps1.score > ps2.score;
	};

	bool cmpPidScore2(const PidScore &ps1, const PidScore &ps2)
	{
		return ps1.pid < ps2.pid;
	};

	enum AttribType{ALL,PID,FINGERPRINT,WIDTH,HEIGHT,SIFTNUM,SEARCHSCORE};
	//image key --> Image attribute
	struct ImageAttrib
	{
		private:
			std::bitset<16> attrib;
		public:
			std::string pid;
			unsigned long long fingerprint;
			int width;
			int height;
			int siftnum;
			int searchscore;

			//AttribType attribtype;

			ImageAttrib()
			{
				attrib.reset();
				reset();
			};
			~ImageAttrib()
			{
			};

			void reset()
			{
				pid = "";
				width = 0;
				height = 0;
				siftnum = 0;
				fingerprint = 0;
				searchscore = 0;
			};

			bool attribOn(AttribType type)
			{
				if(type == ALL)
					attrib.set();
				else
					attrib.set(type);

				return true;
			};

			bool attribOff(AttribType type)
			{
				if(type == ALL)
					attrib.reset();
				else
					attrib.reset(type);

				return true;
			};

			bool testAttrib(AttribType type)
			{
				if(type == ALL)
				{
					if(attrib.count() == 16)
						return true;
					else
						return false;
				}
				else
					return  attrib.test(type);
			};
	};

	//Implemention of multi-hash indexing using redis
	class MultiIndex
	{
		redisContext *c; //local redis client
		//	redisReply *reply; //redis reply

		Log *log;  //log

		std::vector<unsigned> flipvec;
	//	boost::mutex io_mutex;
	//	boost::mutex map_mutex;
public:
	
		MultiIndex()
		{
			c = NULL;
	//		reply = NULL;

			log = NULL;

			flipvec.resize(32);
			for(int i=0;i < 32; i++)
			{
				flipvec[i] = 1 << i;
			}
		};
		
		~MultiIndex()
		{
			free();
		};

		void free()
		{
			//		if(reply)freeReplyObject(reply);
			if(c)redisFree(c);
			c = NULL;
		};

		//timoout in seconds
		bool redisConnect(char *ip = "127.0.0.1",int port = 6379,int  timeout_sec = 0,int timeout_usec = 100000)
		{
			struct timeval t;
			t.tv_sec = timeout_sec;
			t.tv_usec = timeout_usec;

			free();
			c = redisConnectWithTimeout(ip, port, t);
			if (c == NULL || c->err) {
				if (c) {
					printf("failed to connect to redis server %s %d , error type: %s\n", ip, port, c->errstr);
					free();
				} else {
					printf("redis connection error: can't allocate redis context\n");
				}
				return false;
			}
			
			return true;
		};

		bool redisSetDB(int idx)
		{
			redisReply *reply;
		
			reply = (redisReply *)redisCommand(c, "select %d", idx);

			if(!reply)return false;

			if(strcmp("OK", reply->str) == 0)
			{
				freeReplyObject(reply);
				return true;
			}
			else
			{
				freeReplyObject(reply);
				return false;
			}
		};

		//global setting using redis get and set
		bool redisGet(char * key, int keysize, char *value, int &valuesize)
		{
			redisReply *reply;
			redisSetDB(8);

			reply = (redisReply *)redisCommand(c, "get %b", key, keysize);
			if(!reply)return false;

			if(reply->type == REDIS_REPLY_STRING)
			{
				strncpy(value, reply->str, reply->len);
				valuesize = reply->len;

				freeReplyObject(reply);
				return true;
			}
			else
			{
				freeReplyObject(reply);
				return false;
			}

		};

		bool redisSet(char * key, int keysize, char *value, int valuesize)
		{
			redisReply *reply;
			redisSetDB(8);

			reply = (redisReply *)redisCommand(c, "set %b %b", key, keysize, value, valuesize);
			if(!reply)return false;

			if(strcmp("OK", reply->str) == 0)
			{
				freeReplyObject(reply);
				return true;
			}
			else
			{
				freeReplyObject(reply);
				return false;
			}
		};

		void setLog(Log * index_log = NULL)
		{
			log = index_log;
		};

		unsigned getKey(Point &pt, int idx)
		{
			unsigned * c1 = (unsigned *) pt.desc.sketch;
			return c1[idx];
		};	

		//get sketch from imageid
		int getSketchID(Key &imageid, std::vector<Key> &sketchid)
		{
			//redisReply *reply;
			
			redisSetDB(5);
			
			ImageAttrib attrib;
			attrib.attribOn(SIFTNUM);

			queryImageAttrib(imageid, attrib);

			imageid = imageid << 8;
			for(int i=0;i<attrib.siftnum;i++)
			{
				Key tempkey = imageid + (Key)i;
				sketchid.push_back(tempkey);
			}

			return attrib.siftnum;
		};

		//get Sketch
		int getSketch(Key imageid, std::vector<Point> &sketch)
		{
			std::vector<Key> sketchid;
			getSketchID(imageid, sketchid);

			redisReply *reply;

			redisSetDB(4);

			Point pt;

			for(int i=0;i<sketchid.size();i++)
			{
				reply = (redisReply *)redisCommand(c, "get %b", (char *)(&sketchid[i]), sizeof(Key));
				if(!reply)continue;
	
				if(reply->type = REDIS_REPLY_STRING)
				{
					pt.key = sketchid[i];
					pt.desc = *((BitFeature *)reply->str);

					sketch.push_back(pt);
				}
				freeReplyObject(reply);
			}

			return sketch.size();
		};

		bool checkImageID(Key &imageid)
		{
			redisReply *reply;

			redisSetDB(5);

			reply = (redisReply *)redisCommand(c, "exists %b", (char *)&imageid, sizeof(Key));
			if(!reply)return false;

			if(reply->type == REDIS_REPLY_INTEGER)
			{
				if(reply->integer == 1)
				{
					freeReplyObject(reply);
					return true;
				}
				else
				{
					freeReplyObject(reply);
					return false;
				}
			}

			freeReplyObject(reply);
			return false;

		};

		int checkImagePID(char *pid, int pidlen)
		{
			redisReply *reply;

			redisSetDB(6);

			reply = (redisReply *)redisCommand(c, "exists %b", pid, pidlen);
			if(!reply)return -1;
			if(reply->type == REDIS_REPLY_INTEGER)
			{
				if(reply->integer == 1)
				{
					freeReplyObject(reply);
					return 1;
				}
				else
				{
					freeReplyObject(reply);
					return 0;
				}
			}

			freeReplyObject(reply);
			return -1;
		};

		int checkFingerPrint(unsigned long long &fingerprint)
		{
			redisReply *reply;

			redisSetDB(7);

			reply = (redisReply *)redisCommand(c, "exists %b", (char *)&fingerprint, sizeof(unsigned long long));
			if(!reply)return -1;	
			if(reply->type == REDIS_REPLY_INTEGER)
			{
				if(reply->integer == 1)
				{
					freeReplyObject(reply);
					return 1;
				}
				else
				{
					freeReplyObject(reply);
					return 0;
				}
			}

			freeReplyObject(reply);
			return -1;
		};

		bool insertFingerPrint(unsigned long long fingerprint, Key key)
		{
			redisReply *reply;

			redisSetDB(7);

			reply = (redisReply *)redisCommand(c, "rpush %b %b", (char *)&fingerprint, sizeof(fingerprint), (char*)&key, sizeof(Key));
			if(reply)freeReplyObject(reply);

			return true;
		};

		bool deleteFingerPrint(unsigned long long fingerprint, Key key, bool clear = false)
		{
			redisReply *reply;
			
			redisSetDB(7);

			if(clear)
				reply = (redisReply *)redisCommand(c, "del %b", (char *)&fingerprint, sizeof(fingerprint));
			else
				reply = (redisReply *)redisCommand(c, "lrem %b -1 %b", (char *)&fingerprint, sizeof(fingerprint), (char *)&key, sizeof(key));
			if(reply)freeReplyObject(reply);

			return true;
		};

		int queryFingerPrint(unsigned long long fingerprint, std::vector<Key> &keys)
		{
			redisReply *reply;

			redisSetDB(7);

			reply = (redisReply *)redisCommand(c, "lrange %b 0 -1", (char *)&fingerprint, sizeof(fingerprint));
			if(!reply)return 0;

			if (reply->type == REDIS_REPLY_ARRAY) {
				for (int j = 0; j < reply->elements; j++) {
					keys.push_back(*((Key *)reply->element[j]->str));
				}
			}

			freeReplyObject(reply);

			return keys.size();
		};

		bool insertImagePID(char *pid, int pidlen, Key key)
		{
			redisReply *reply;

			redisSetDB(6);

			reply = (redisReply *)redisCommand(c, "rpush %b %b", pid, pidlen, (char *)&key, sizeof(Key));
			if(reply)freeReplyObject(reply);

			return true;
		};

		 bool deleteImagePID(char *pid, int pidlen, Key key,bool clear = false)
		 {
			 redisReply *reply;

			 redisSetDB(6);

			 if(clear)
				 reply = (redisReply *)redisCommand(c, "del %b", pid, pidlen);
			 else
				 reply = (redisReply *)redisCommand(c, "lrem %b -1 %b", pid, pidlen, (char *)&key, sizeof(key));
			 if(reply)freeReplyObject(reply);
			 return true;
		 };

		 int queryImagePID(char *pid, int pidlen, std::vector<Key> &keys)
		 {
			 redisReply *reply;

			 redisSetDB(6);

			 reply = (redisReply *)redisCommand(c, "lrange %b 0 -1",pid, pidlen);
			 if(!reply)return 0;

			 if (reply->type == REDIS_REPLY_ARRAY) {
				 for (int j = 0; j < reply->elements; j++) {
					 keys.push_back(*((Key *)reply->element[j]->str));
				 }
			 }

			freeReplyObject(reply);

			return keys.size();
		 };

		 bool insertImageAttrib(Key & key, ImageAttrib & attrib)
		 {
			 redisReply *reply;

			 redisSetDB(5);

			 char field;

			 if(attrib.testAttrib(PID))
			 {	
				 field = PID;
				 reply = (redisReply *)redisCommand(c, "hset %b %b %b", (char *)&key, sizeof(Key), &field, 1, attrib.pid.data(), attrib.pid.size());
				 if(reply)freeReplyObject(reply);
			 }

			 if(attrib.testAttrib(FINGERPRINT))
			 {	
				 field = FINGERPRINT;
				 reply = (redisReply *)redisCommand(c, "hset %b %b %b", (char *)&key, sizeof(Key), &field, 1, (char *)&attrib.fingerprint, sizeof(attrib.fingerprint));
				if(reply)freeReplyObject(reply);
			 }

			 if(attrib.testAttrib(WIDTH))
			 {	
				 field = WIDTH;
				 reply = (redisReply *)redisCommand(c, "hset %b %b %b", (char *)&key, sizeof(Key), &field, 1, (char *)&attrib.width, sizeof(attrib.width));
				 if(reply)freeReplyObject(reply);
			 }

			 if(attrib.testAttrib(HEIGHT))
			 {	
				 field = HEIGHT;
				 reply = (redisReply *)redisCommand(c, "hset %b %b %b", (char *)&key, sizeof(Key), &field, 1, (char*)&attrib.height, sizeof(attrib.height));
				 if(reply)freeReplyObject(reply);
			 }

			 if(attrib.testAttrib(SIFTNUM))
			 {
				 field = SIFTNUM;
				 reply = (redisReply *)redisCommand(c, "hset %b %b %b", (char *)&key, sizeof(Key), &field, 1, (char *)&attrib.siftnum, sizeof(attrib.siftnum));
				 if(reply)freeReplyObject(reply);
			 }

			 if(attrib.testAttrib(SEARCHSCORE))
			 {
				 field = SEARCHSCORE;
				 reply = (redisReply *)redisCommand(c, "hset %b %b %b", (char *)&key, sizeof(Key), &field, 1, (char *)&attrib.searchscore, sizeof(attrib.searchscore));
				 if(reply)freeReplyObject(reply);
			 }

			 return true;
		 };

		bool deleteImageAttrib(Key & key)
		{
			redisReply *reply;

			redisSetDB(5);

			reply = (redisReply *)redisCommand(c, "del %b", (char *)&key, sizeof(Key));
		
			if(reply)freeReplyObject(reply);
		
			return true;
		};


		bool queryImageAttrib(Key & key, ImageAttrib & attrib)
		{
			redisReply *reply;

			redisSetDB(5);

			char field;

			if(attrib.testAttrib(PID))
			{
				field = PID;
				reply = (redisReply *)redisCommand(c, "hget %b %b", (char *)&key, sizeof(Key), &field, 1);
				if(reply && reply->type == REDIS_REPLY_STRING)attrib.pid.assign(reply->str, reply->len);
				if(reply)freeReplyObject(reply);
			}

			if(attrib.testAttrib(FINGERPRINT))
			{
				field = FINGERPRINT;
				reply = (redisReply *)redisCommand(c, "hget %b %b", (char *)&key, sizeof(Key), &field, 1);
				if(reply && reply->type == REDIS_REPLY_STRING)attrib.fingerprint = *((unsigned long long *)reply->str);
				if(reply)freeReplyObject(reply);
			}

			if(attrib.testAttrib(WIDTH))
			{
				field = WIDTH;
				reply = (redisReply *)redisCommand(c, "hget %b %b", (char *)&key, sizeof(Key), &field, 1);
				if(reply && reply->type == REDIS_REPLY_STRING)attrib.width = *((int *)reply->str);
				if(reply)freeReplyObject(reply);
			}

			if(attrib.testAttrib(HEIGHT))
			{
				field = HEIGHT;
				reply = (redisReply *)redisCommand(c, "hget %b %b", (char *)&key, sizeof(Key), &field, 1);
				if(reply && reply->type == REDIS_REPLY_STRING)attrib.height =  *((int *)reply->str);

				if(reply)freeReplyObject(reply);
			}

			if(attrib.testAttrib(SIFTNUM))
			{
				field = SIFTNUM;
				reply = (redisReply *)redisCommand(c, "hget %b %b", (char *)&key, sizeof(Key), &field, 1);
				if(reply && reply->type == REDIS_REPLY_STRING)attrib.siftnum =  *((int *)reply->str);

				if(reply)freeReplyObject(reply);
			}

			if(attrib.testAttrib(SEARCHSCORE))
			{
				field = SEARCHSCORE;
				reply = (redisReply *)redisCommand(c, "hget %b %b", (char *)&key, sizeof(Key), &field, 1);
				if(reply && reply->type == REDIS_REPLY_STRING)attrib.searchscore =  *((int *)reply->str);

				if(reply)freeReplyObject(reply);
			}

			return true;
		};

		int getSiftNum(Key imageid)
		{
			int siftnum = 0;

			redisReply *reply;

			redisSetDB(5);

			char field = SIFTNUM;

			reply = (redisReply *)redisCommand(c, "hget %b %b", (char *)&imageid, sizeof(Key), &field, 1);

			if(reply && reply->type == REDIS_REPLY_STRING && reply->len == sizeof(int))siftnum =  *((int *)reply->str);

			if(reply)freeReplyObject(reply);

			return siftnum;
		};

		int getPid(Key imageid, std::string &pid)
		{
			redisReply *reply;

			redisSetDB(5);

			char field = PID;

			reply = (redisReply *)redisCommand(c, "hget %b %b", (char *)&imageid, sizeof(Key), &field, 1);

			pid = "";

			if(reply && reply->type == REDIS_REPLY_STRING) pid.assign(reply->str, reply->len);

			if(reply)freeReplyObject(reply);

			return pid.size();
		};

		unsigned long long getFingerPrint(Key imageid)
		{
			unsigned long long fp=0;

			redisReply *reply;

                        redisSetDB(5);

                        char field = FINGERPRINT;

                        reply = (redisReply *)redisCommand(c, "hget %b %b", (char *)&imageid, sizeof(Key), &field, 1);

                        if(reply && reply->type == REDIS_REPLY_STRING) fp = *((unsigned long long *)reply->str);

                        if(reply)freeReplyObject(reply);

                        return fp;
		};

		bool insertPoint(Point &pt)
		{
			redisReply *reply;
			bool bOK = true;

			redisSetDB(4); //sketchid sketch K/V 
			reply = (redisReply *)redisCommand(c, "set %b %b", (unsigned char *)&pt.key, sizeof(pt.key), (unsigned char *)pt.desc.sketch, sizeof(pt.desc));
			
			if(reply)freeReplyObject(reply);

			//if(!bOK)return false;

			for(int i=0; i < 4; i++)
			{
				redisSetDB(i);
				
				unsigned indexkey = getKey(pt, i);

				reply = (redisReply *)redisCommand(c, "rpush %b %b", (unsigned char *)&indexkey,sizeof(unsigned), (unsigned char *)&pt.key, sizeof(pt.key));
				if(reply)freeReplyObject(reply);
			};

			return true;
		};

		bool deletePoint(Point &pt)
		{
			redisReply *reply;
			for(int i=0; i < 4; i++)
			{
				redisSetDB(i);

				unsigned indexkey = getKey(pt, i);

				reply = (redisReply *)redisCommand(c, "lrem %b -1 %b", (unsigned char *)&indexkey,sizeof(unsigned), (unsigned char *)&pt.key, sizeof(pt.key));
				if(reply)freeReplyObject(reply);
			};

			redisSetDB(4); //sketchid sketch K/V 
			reply = (redisReply *)redisCommand(c, "del %b", (unsigned char *)&pt.key, sizeof(pt.key));
			if(reply)freeReplyObject(reply);

			return true;
		};

		bool insertRecord(Record &record, Key imageid)
		{
			Point pt;
			imageid = imageid << 8;

			for(int i=0; i < record.desc.size();i++)
			{
				pt.desc = record.desc[i];
				pt.key = (Key)i + imageid;
				
				insertPoint(pt);
			}

			if(log)log->log("Index::insertRecord finished.");

			return true;
		};

		bool deleteRecord(Record &record, Key imageid)
                {
                        Point pt;
                        imageid = imageid << 8;

                        for(int i=0; i < record.desc.size();i++)
                        {
                                pt.desc = record.desc[i];
                                pt.key = (Key)i + imageid;

                                deletePoint(pt);
                        }

                        if(log)log->log("MultiIndex::deleteRecord finished.");

                        return true;
                };

		bool searchPoint(Point &pt, std::vector<Key> &result, int hamthres = 7, bool flip = true)
		{
			Hamming hamming;
			std::set<Key> candiKeys;

			redisReply *reply;
			//get candidate keys from redis db
			for(int i=0; i < 4; i++)
			{
				redisSetDB(i);

				unsigned indexkey = getKey(pt, i);

				std::vector<unsigned> keys;
				keys.push_back(indexkey);

				if(flip)
				{	unsigned tempkey;
					for(int i=0;i<32;i++)
					{
						tempkey = indexkey ^ flipvec[i];
						keys.push_back(tempkey);
					}
				}

				for(int i=0;i < keys.size();i++)
				{
					redisAppendCommand(c,"LRANGE %b 0 -1", (unsigned char *)&keys[i], sizeof(unsigned));
				}

				for(int i=0;i < keys.size();i++)
				{
					if(REDIS_OK == redisGetReply(c, (void **)&reply))
						if (reply && reply->type == REDIS_REPLY_ARRAY) {
							for (int j = 0; j < reply->elements; j++) {
								candiKeys.insert(*((Key *)reply->element[j]->str));
							}
						}
					if(reply)freeReplyObject(reply);
				}
			};

			//refine the candidate keys
			redisSetDB(4);//get sketch from keys
			std::set<Key>::const_iterator b = candiKeys.begin();
			Key tempkey = 0;
			for(; b!= candiKeys.end(); ++b)
			{
				tempkey = *b;
				redisAppendCommand(c, "get %b", (unsigned char *)&tempkey, sizeof(tempkey));
			}

			b = candiKeys.begin();
			for(; b != candiKeys.end(); ++b)
			{
				tempkey = *b;
				if(REDIS_OK == redisGetReply(c, (void **)&reply))
				if(reply && reply->type == REDIS_REPLY_STRING)
					if(hamming(pt.desc.sketch, (Chunk *)reply->str) <= hamthres){
						tempkey = tempkey >> 8; //get image id from sketch id
						result.push_back(tempkey);
					}
				if(reply)freeReplyObject(reply);	
			}

			return true;
		};

		bool searchRecord(Record &record, std::vector<Key> & result, int hamthres = 7, bool flip = true)
		{
			Point pt;	

			for(int i=0;i< record.desc.size();i++)
			{
				pt.desc = record.desc[i];
				searchPoint(pt, result, hamthres, flip);
			}
			if(log)log->log("MultiIndex::searchRecord finished.");
			return true;
		};

		int rankSearchResult(std::vector<Key> & searchresult, std::vector<KeyScore> &rankresult, float thresh = 0.05, int minsift = 2)
		{
			std::sort(searchresult.begin(), searchresult.end());
	
			KeyScore ks;
			Key curkey, nextkey;

			for(int i=0;i< searchresult.size();i++)
			{
				curkey = searchresult[i];

				if(i+1 < searchresult.size())
					nextkey = searchresult[i+1];
				else
					nextkey = 0;

				ks.score++;

				if(curkey != nextkey)
				{
					int siftnum = getSiftNum(searchresult[i]);

					if(siftnum >= minsift && ks.score > 1.0)
					{
						ks.key = searchresult[i];
						ks.score /= siftnum;

						if(ks.score > 1.0) ks.score = 1.0;

						if(ks.score > thresh)rankresult.push_back(ks);
					}

					ks.reset();
				}
			}

			std::sort(rankresult.begin(), rankresult.end(), cmpKeyScore);

			return rankresult.size();
		};

		int rankKeyScore(std::vector<KeyScore> &rankresult, std::vector<PidScore> &mergeresult, bool mergepid = false)
		{
			PidScore pidscore;

			for(int i=0; i<rankresult.size(); i++)
			{
				getPid(rankresult[i].key, pidscore.pid);
				pidscore.score = rankresult[i].score;

				if(pidscore.pid != "")mergeresult.push_back(pidscore);
			}

			if(mergepid)
			{
			//	std::sort(mergeresult.begin(), mergeresult.end(), cmpPidScore2);
			}
			
			return mergeresult.size();
		};
	};
};

#endif
