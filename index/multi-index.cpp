//multi index test program

#include <iostream>
#include <vector>
#include <string>

#include "../common/record.h"
#include "../common/eval.h"
#include "../common/socket.h"
#include "../common/nisereply.h"
#include "../extract/feature_extractor.h"

#include "multi-index.h"

using namespace std;
using namespace nise;

int main(int argc, char ** argv)
{
	if(argc != 3)
	{
		cout << "Usage : multi-index <insert/delete/query> <image path>" << endl;
		return 0;
	};
	FeatureExtractor xtor;
	MultiIndex mindex;

	vector<BitFeat> f;

	string img_path(argv[2]);

	Record record;
	NiseReply reply;
	Timer timer;
	float time;

	if(xtor.loadImage(img_path))
	{
		timer.restart();
		xtor.getBitFeat(f, LDATEST, 1, 1000);
		for(int i=0;i<f.size();i++)record.desc.push_back(f[i].f);
		record.fingerprint = xtor.getFingerPrint();
		record.img_url = img_path;
		time = timer.elapsed();

		cout << "Time cost for feature extraction " << time << " seconds." << endl;
		
		timer.restart();
		
		Socket sock;

		if(strcasecmp(argv[1], "insert")==0)
		{
			int connfd = sock.connectTo("127.0.0.1", 9999);
			record.sendRecord(connfd);
			reply.recvReply(connfd);
			if(reply.replytype == REPLY_OK)
			{
				cout << "Image pid " << record.img_url << endl;
				cout << "Image fingerprint " << record.fingerprint << endl;
				cout << f.size() << " sketches inserted." << endl;
			}
			else
				cout << "Image insert failed." << endl;
		}

		if(strcasecmp(argv[1], "delete")==0)
		{
			int connfd = sock.connectTo("127.0.0.1", 11111);
			record.sendRecord(connfd);
			reply.recvReply(connfd);
			if(reply.replytype == REPLY_OK)
			{
				cout << "Image pid " << record.img_url << endl;
                                cout << "Image fingerprint " << record.fingerprint << endl;
				cout << f.size() << " sketches deleted." << endl;
			}
			else
				cout << "Image delete failed." << endl;
		}

		if(strcasecmp(argv[1], "query")==0)
		{
			int connfd = sock.connectTo("127.0.0.1", 8888);
			record.sendRecord(connfd);
			reply.recvReply(connfd);
			if(reply.replytype == REPLY_OK)
			{
				cout << f.size() << " sketches searched." << endl;
				cout << reply.pidscore.size() << " search result found." << endl;
				for(int i=0;i<reply.pidscore.size();i++)
					cout << reply.pidscore[i].pid << "\t" << reply.pidscore[i].score << "\t";
				cout << endl;
			}
			else
				cout << "Image search failed." << endl;
		}

		time = timer.elapsed();
		cout << "Time cost for redis operation : " << time << " seconds." << endl;
	}
	else
	{
		cout << "Failed to load image " << argv[2] << endl; 
		return 0;
	}

	return 1;
};
