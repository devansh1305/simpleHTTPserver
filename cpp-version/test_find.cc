#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <cstring>

using namespace std;

int main(){
	string test="Devansh /GET pop/dev";
	while((int a=test.find("/"))!=string::npos){
		printf("lol: %d\n",a);
	}
	int pos1=test.find("/");
	int pos2=test.find("/",1);
//	cout<<"string: "<<test<<endl<<"pos1: "<<pos1<<endl<<"pos2: "<<pos2<<endl;
}
