#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <vector>

using namespace std;

string stat_name="Devansh Panirwala";
long min_e;
long max_e;
struct timespec before, after;
long elapsed_nsecs;
int nrequests=0;

struct OBJ_file {
	string name;
	string time;
};

int QueueLength = 5;

// Processes time request
void processTimeRequest( int socket );
void parseMyClientData(string request,int sock);
void poolSlave(int socket);
void processRequestThread(int socket);

pthread_mutex_t mt;
pthread_mutexattr_t mattr;


extern "C" void killzombie(int sig){
	int pid = wait3(0, 0, NULL);
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main( int argc, char ** argv ){
	// Add your HTTP implementation here
	// Print usage if not enough arguments

	// Get the port from the arguments
	int port;
	int thr_mode=0;
	if(argc==1){
		port=1200;
	}
	else if(argc==2){
		port = atoi( argv[1] );
	}
	else if(argc==3){
		port = atoi(argv[2]);

		if (strcmp(argv[1],"-f")==0){
			thr_mode=1;
		}
		else if(strcmp(argv[1],"-t")==0){
			thr_mode=2;
		}
		else if(strcmp(argv[1],"-p")==0){
			thr_mode=3;
		}
		else{
			cout<<"INVALID ARGUMENTS!!"<<endl;
			exit(-1);
		}
	}
	else{
		cout<<"INVALID ARGUMENTS!!"<<endl;
		exit(-1);
	}

	struct sigaction sigAction;
	sigAction.sa_handler = killzombie;
	sigemptyset(&sigAction.sa_mask);
	sigAction.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sigAction, NULL)){
		perror("sigaction");
		exit(-1);
	}

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);

	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if ( masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			(char *) &optval, sizeof( int ) );

	// Bind the socket to the IP address and port
	int error = bind( masterSocket,
			(struct sockaddr *)&serverIPAddress,
			sizeof(serverIPAddress) );
	if ( error ) {
		perror("bind");
		exit( -1 );
	}

	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen( masterSocket, QueueLength);
	if ( error ) {
		perror("listen");
		exit( -1 );
	}
	if (thr_mode == 3){            
		pthread_mutexattr_init(&mattr);
		pthread_t tid[5];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);            
		pthread_mutex_init(&mt, &mattr);
		for(int i=0;i<5;i++){
			pthread_create(&tid[i],&attr,(void*(*)(void*))poolSlave,(void*)masterSocket);
		}
		pthread_join(tid[0], NULL);
	}else{
		while ( 1 ) {

			// Accept incoming connections
			struct sockaddr_in clientIPAddress;
			int alen = sizeof( clientIPAddress );
			int slaveSocket = accept( masterSocket,
					(struct sockaddr *)&clientIPAddress,
					(socklen_t*)&alen);

			if(thr_mode==1){
				pid_t slave=fork();
				if(slave==0){
					processTimeRequest(slaveSocket);
					close(slaveSocket);
					exit(1);
				}
				close(slaveSocket);
			}
			else if(thr_mode==2){
				pthread_t tid;
				pthread_attr_t attr;
				pthread_attr_init(&attr);
				pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
				pthread_create(&tid,&attr,(void*(*)(void*))processRequestThread,(void*)slaveSocket);
			}
			else if(thr_mode==0){
				if ( slaveSocket < 0 ) {
					perror( "accept" );
					exit( -1 );
				}
				// Process request.
				processTimeRequest( slaveSocket );

				// Close socket
				close( slaveSocket );
			}
		}
	}
}

void processRequestThread(int socket) {
	processTimeRequest(socket);
	close(socket);
}


void poolSlave(int socket) {
	while(1){
		struct sockaddr_in clientIPAddress;
		int alen=sizeof(clientIPAddress);
		pthread_mutex_lock(&mt);
		int slaveSocket = accept(socket,(struct sockaddr*)&clientIPAddress,(socklen_t*)&alen);
		pthread_mutex_unlock(&mt);
		clock_gettime(CLOCK_REALTIME, &before);
		processTimeRequest(slaveSocket);
		clock_gettime(CLOCK_REALTIME, &after);
		elapsed_nsecs = (after.tv_sec - before.tv_sec) * 1000000000 + (after.tv_nsec - before.tv_nsec);
		if(elapsed_nsecs>max_e) max_e=elapsed_nsecs;
		if(elapsed_nsecs<min_e) min_e=elapsed_nsecs;
		nrequests++;
		close(slaveSocket);
	}
}

void processTimeRequest( int fd ){
	const int MaxName = 1024;
	string client_data="";
	int data_length = 0;
	int n;
	unsigned char newChar;
	unsigned char lastChar = 0;
	while ( data_length < MaxName && ( n = read( fd, &newChar, sizeof(newChar)))) {
		if(client_data.length()>4){
			string endloop=client_data.substr(client_data.length()-3);
			endloop+=newChar;
			if(endloop=="\r\n\r\n"){
				break;
			}
		}
		client_data+=newChar;
		data_length++;
		lastChar = newChar;
	}
	//client_data.pop_back();
	//client_data.pop_back();
	//client_data.pop_back();
	if(client_data.find("Authorization: Basic ZGV2OmRldjEzMDU=")==string::npos){
		string Oauth="HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"myhttpd-cs252\"";
		write(fd,Oauth.c_str(),strlen(Oauth.c_str()));
	}
	else{
		parseMyClientData(client_data,fd);
	}
}

string getFileExt(string dir_path){
	string Ctype;
	if(dir_path.find(".html")!=string::npos){
		Ctype="text/html";
	}
	else if(dir_path=="/dev1305/"){
		Ctype="text/html";
	}
	else if(dir_path.find(".gif")!=string::npos){
		Ctype="image/gif";
	}
	else if(dir_path.find(".jpg")!=string::npos){
		Ctype="image/jpeg";
	}
	else if(dir_path.find(".png")!=string::npos){
		Ctype="image/png";
	}
	else if(dir_path.find(".svg")!=string::npos){
		Ctype="image/svg+xml";
	}
	else{
		Ctype="text/html";
	}
	return Ctype;

}

bool comparator_N_A (const OBJ_file &a, const OBJ_file &b){
	return a.name<b.name;
}
bool comparator_N_D (const OBJ_file &a, const OBJ_file &b){
	return a.name>b.name;
}
bool comparator_M_A (const OBJ_file &a, const OBJ_file &b){
	return a.time<b.time;
}
bool comparator_M_D (const OBJ_file &a, const OBJ_file &b){
	return a.time>b.time;
}

string buildDIRpage(vector<OBJ_file> file_names){
	string page="";
	string lol="<tr><th valign=\"top\"><img src=\"/icons/blank.gif\" alt=\"[ICO]\"></th><th><a href=\"?C=N;O=D\">Name</a></th><th><a href=\"?C=M;O=A\">Last modified</a></th></tr>";
	page+="<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\"><html><head><title>CS422: HTTP Server</title></head><body><h1>CS252: HTTP Server</h1><table>";
	page+=lol;
	for(int i=0;i<file_names.size();i++){
		string img;
		if(file_names[i].name.find("dir")!=string::npos){
			img="<td valign=\"right\"><img src=\"menu.gif\" alt=\"[DIR]\"></td>";
		}
		else if(file_names[i].name.find("gif")!=string::npos){
			img="<td valign=\"right\"><img src=\"image.gif\" alt=\"[IMG]\"></td>";
		}
		else{
			img="<td valign=\"right\"><img src=\"unknown.gif\" alt=\"[   ]\"></td>";
		}
		string origin=img+"<tr><td valign=\"top\"></td><td>";
		string dirLine="<a href=";
		dirLine+="\"" + file_names[i].name + "\">" + file_names[i].name + "</a></td><td align=\"right\">"+ file_names[i].time  +" </td>";
		origin+=dirLine;
		page+=origin;
	}
	page+="</table><hr></body></html>";
	return page;
}
string getArgQuery(string Cpath){
	int quest=Cpath.find("?");
	if(quest!=string::npos){
		string qstr=Cpath.substr(quest+1,Cpath.size()-1);
		return qstr;
	}
	else{
		return "XX_NONE_XX";
	}
}

void req_log(string data){
	FILE *fp = fopen("mystat", "a");
	if (fp != NULL)
	{
		fputs(data.c_str(), fp);
		fclose(fp);
	}
}

void parseMyClientData(string request,int sock){
	int firstLine=request.find("\r\n");
	string tempRequest=request.substr(0,firstLine);
	cout << "tempReq: "<<tempRequest<<endl;
	string pass;
	int getSecretPos=tempRequest.find("/dev1305/");
	printf("Pos: %d\n",getSecretPos);
	if(string::npos!=getSecretPos){
		int _httppos=tempRequest.find("HTTP");
		string dir_path=tempRequest.substr(getSecretPos,_httppos-5);
		cout << "htp pos: " << dir_path << endl;

		string Ctype=getFileExt(dir_path);

		pass="HTTP/1.0 200 Document follows\r\nServer: CS_252_lab5\r\nContent-Type: "+Ctype+"\r\n\r\n";

		char cwd[256];
		getcwd(cwd,sizeof(cwd));
		string req_log1="http:"+to_string(sock)+":"+to_string(8843)+dir_path + "\n";
		req_log(req_log1);
		nrequests++;
		if(dir_path.find("stat")!=string::npos){
			pass="HTTP/1.0 200 Document follows\r\nServer: CS_252_lab5\r\nContent-type: text/html\r\n\r\n<meta charset=\"utf-8\"/><html><body><h1>MYSTATS</h1>";
			pass+="\r\nStudent Name: "+stat_name+"\r\n";
			pass+="Minimum Time: " + to_string(min_e)+"\r\n";
			pass+="Maximum Time: " + to_string(max_e)+"\r\n";
			pass+="No of Requests: " + to_string(nrequests)+"\r\n";
			pass+="</body></html>";

			write(sock,pass.c_str(),pass.size());
			return;
		}
		if(dir_path=="/dev1305/log"){
			pass="HTTP/1.0 200 Document follows\r\nServer: CS_252_lab5\r\nContent-type: text/html\r\n\r\n<meta charset=\"utf-8\"/><html><body><h1>MYSTATS</h1>";
			ifstream ifs("mystat");
			string content((std::istreambuf_iterator<char>(ifs)),
					(std::istreambuf_iterator<char>()));
			pass+=content;
			pass+="</body></html>";
			write(sock,pass.c_str(),strlen(pass.c_str()));
			return;
		}
		if(dir_path=="/dev1305/"){
			//string Cpath=cwd;
			//Cpath+="/http-root-dir/";
			ifstream ifs("http-root-dir/htdocs/index.html");
			string content((std::istreambuf_iterator<char>(ifs)),
					(std::istreambuf_iterator<char>()));
			pass+=content;
			write(sock,pass.c_str(),strlen(pass.c_str()));

		}else{
			write(sock,pass.c_str(),strlen(pass.c_str()));
			string Cpath=dir_path.substr(8,dir_path.size());
			string test_l=Cpath;
			int quest1=test_l.find("?");
			if(quest1!=string::npos){
				Cpath=dir_path.substr(8,quest1);
			}
			//cout<<endl<<"DIRPATH: "<<dir_path<<endl;
			//cout<<endl<<"CPATH: "<<Cpath<<endl;
			Cpath="http-root-dir/htdocs"+Cpath;
			cout<<"Cpath2: "<< Cpath <<endl;

			DIR *dir;
			struct dirent *drnt;
			dir=opendir(Cpath.c_str());

			if(dir!=NULL){
				vector<OBJ_file> objdir;
				while(drnt = readdir(dir)){
					struct OBJ_file filedata;
					struct stat filestat;
					filedata.name=drnt->d_name;
					stat((filedata.name).c_str(),&filestat);
					struct tm * timeinfo = localtime(&filestat.st_ctime);
					filedata.time=asctime(timeinfo);
					objdir.push_back(filedata);
				}
				//closedir(dir);
				string qstr=getArgQuery(dir_path);
				cout<<"qstr: "<<qstr<<endl;
				if(qstr=="C=N;O=D"){
					sort(objdir.begin(), objdir.end(), comparator_N_D);
					cout<<"PART1"<<endl;
				}
				else if(qstr=="C=N;O=A"){
					sort(objdir.begin(), objdir.end(), comparator_N_A);
					cout<<"PART2"<<endl;
				}
				else if(qstr=="C=M;O=A"){
					sort(objdir.begin(), objdir.end(), comparator_M_A);
					cout<<"PART3"<<endl;
				}
				else if(qstr=="C=M;O=D"){
					sort(objdir.begin(), objdir.end(), comparator_M_D);
					cout<<"PART4"<<endl;
				}
				else{
					sort(objdir.begin(), objdir.end(), comparator_N_A);
				}

				string dir_page=buildDIRpage(objdir);
				pass+=dir_page;
				write(sock,pass.c_str(),strlen(pass.c_str()));

				return;

			}
			else if(Cpath.find("cgi-bin")!=string::npos){
				//cout<<endl<<"Cpath_cgi: "<< Cpath <<endl<<endl;
				int ret;
				ret=fork();
				if(ret==0){
					string pass="HTTP/1.1 200 Document follows\r\nServer: CS_252_lab5\r\n";
					//write(sock,pass.c_str(),strlen(pass.c_str()));

					dup2(sock, 1);
					setenv("REQUEST_METHOD","GET",0);
					string file_name;
					if(Cpath.find("test-env")!=string::npos){
						file_name="./http-root-dir/cgi-bin/test-env";
					}
					else if(Cpath.find("test-cgi")!=string::npos){
						file_name="./http-root-dir/cgi-bin/test-cgi";
					}
					else if(Cpath.find("jj")!=string::npos){
						file_name="./http-root-dir/cgi-bin/jj";
					}
					else if(Cpath.find("finger")!=string::npos){
						file_name="./http-root-dir/cgi-bin/finger";
					}
					string qstr=getArgQuery(Cpath);
					if(qstr!="XX_NONE_XX") setenv("QUERY_STRING",qstr.c_str(),0);

					char *cmd = strdup(file_name.c_str());
					char *argv[2];
					argv[0] = strdup(file_name.c_str());
					argv[1] = NULL;
					printf("%s",pass.c_str());
					execvp(cmd,argv);

				}
				else if(ret<0){
					perror("FORK: gone wrong\n");
					return;
				}
			}
			else{
				string content;
				FILE * input;
				char get_char;
				input=fopen(Cpath.c_str(),"rb");
				long int ret=0;
				cout<<"FOPEN: "<<input<<endl;
				while(ret=read(fileno(input),&get_char,sizeof(get_char))>0){
					if (write(sock, &get_char, sizeof(get_char)) != ret) {
						cout<<"END!!"<<endl;
						perror("write");
					}	
				}
				fclose(input);
			}
			closedir(dir);
		}
	}
	else{
		pass="HTTP/1.0 200 Document follows\r\nServer: CS_252_lab5\r\nContent-type: text/html\r\n\r\n<meta charset=\"utf-8\"/><html><body><h1>You do not have permission to acces this server</h1></body></html>";
		write(sock,pass.c_str(),pass.size());
	}
	cout<<"____END OF RESPONSE___"<<endl;
}

