#ifndef _NISE_CLIENT
#define _NISE_CLIENT

#include "../common/socket.h"
#include "../common/nisereply.h"

namespace nise
{
	class NiseClient
	{
	public:
		enum STYPE{IMG_LOCAL, IMG_URL};
		enum RTYPE{};

		Client(){};
		~Client(){};

		int search(char *ip, int port, std::string img, STYPE stype)
		{
			RTYPE rtype;
			Socket sock;
			sock.connectTo(ip, port);
			
			return rtype;
		};
	};
};
#endif
