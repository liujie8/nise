#ifndef _COMMON_FILENAME
#define _COMMON_FILENAME

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <vector>
#include <string>

namespace nise
{
	class FileName
	{
		std::string base_dir;

		public:

		FileName(){};
		~FileName(){};
		
		void setDir(std::string dir)
		{
			base_dir = dir;
		};

		void getFiles(std::vector<std::string> & files)
		{		
			DIR    *dir;
			char    fullpath[1024],currfile[1024];
			struct    dirent    *s_dir;
			struct    stat    file_stat;
			strcpy(fullpath, base_dir.c_str());
			dir = opendir(fullpath);
			if(dir == NULL)
			{
				printf("Failed to open %s.\n", fullpath);
				return;
			}
			while((s_dir = readdir(dir))!=NULL){
				if((strcmp(s_dir->d_name,".")==0)||(strcmp(s_dir->d_name,"..")==0))
					continue;
				sprintf(currfile,"%s/%s",fullpath,s_dir->d_name);
				stat(currfile,&file_stat);
				if(S_ISDIR(file_stat.st_mode))
				{
					base_dir = std::string(currfile);
					getFiles(files);
				}
				else
					files.push_back(std::string(currfile));
			}
			closedir(dir);
		};
	};
};
#endif
