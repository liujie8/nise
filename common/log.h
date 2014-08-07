#ifndef _COMMON_LOG
#define _COMMON_LOG

#include <time.h>
#include <iostream>
#include <string>
#include <fstream>
#include <boost/thread/mutex.hpp>

namespace nise
{
	class Log
	{
		std::ofstream os;
		boost::mutex log_mutex;
	public:
		Log(std::string path = "")
		{
			setPath(path);
		};

		~Log()
		{
			os.close();
		};

		bool setPath(std::string path)
		{
			boost::mutex::scoped_lock lock(log_mutex);
		
			if(os)os.close();
			
			os.open(path.c_str(), std::ios::app);

			if(!os)
			{
                                std::cout << "nise::Log: Failed to open " << path << std::endl;
				return false;
			}

			return true;
		};

		bool log(std::string info)
		{
			boost::mutex::scoped_lock lock(log_mutex);

			if(!os)return false;

			time_t time_stamp;
			time(&time_stamp); //current time
		
			os << ctime(&time_stamp);
			os << info << std::endl;
			os << std::endl;
			return true;
		};
	};
};

#endif
