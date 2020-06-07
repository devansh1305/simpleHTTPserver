#include <algorithm>
#include <sys/stat.h>
#include <time.h>
#include <iostream>

#include <vector>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/dir.h>

using namespace std;

struct OBJ_file {
	string name;
	string time;
};

bool comparator (const OBJ_file &a, const OBJ_file &b){
	return a.time>b.time;
}

int main (){
	vector<OBJ_file> objdir;
	DIR *dir;
	struct dirent *drnt = NULL;
	dir = opendir("./");
	if(dir){
		while(drnt = readdir(dir)){
			struct OBJ_file filedata;
			struct stat filestat;
			filedata.name=drnt->d_name;
			stat((filedata.name).c_str(),&filestat);
			struct tm * timeinfo = localtime(&filestat.st_ctime);
			filedata.time=asctime(timeinfo);
			objdir.push_back(filedata);
		}
		closedir(dir);
	}
	else{
		printf("Can not open directory\n");
	}

	sort(objdir.begin(), objdir.end(), comparator);

	for (int i = 0; i < objdir.size(); i++){
		cout << objdir[i].name << "->" << objdir[i].time;
	}
	return 0;
}
