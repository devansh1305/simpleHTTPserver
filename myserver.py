# python3 myserver.py [port]
from socket import * 
import threading
import sys
import os
import stat
import mimetypes
    
# NOTE _1 flags to print test statements - (1) Print (0) No Print
TEST_FLAG = 1

# NOTE _2 collection of colors to beautify print statements
class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

# NOTE _3
# Discription: builds a directory in HTML format listing all files as anchors links
# Parameters:  directory path
# Returns:     string containing HTML code for directory list
def build_dir(dire):
    dire="./Upload/"+dire
    arr = os.listdir(dire)
    page = "<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML//EN\"><html><head><title>CS422: HTTP Server</title></head><body><h1>CS422: HTTP Server</h1>"
    for files in arr:
        line = "<a href=\"" + files + "\"> " + files + "</a><br>"
        page+=line
    page+="</body></html>"
    return page

# NOTE _4
# Discription: checks if the file has 'others' read permission
# Parameters:  file path
# Returns:     boolean (True - has permission) else False
def isgroupreadable(filepath):
  st = os.stat(filepath)
  return bool(st.st_mode & stat.S_IROTH)

# NOTE _5
# Discription: reads files (encodes it if necessary), calculates size and computes status based on availability of the file
# Parameters:  name of file
# Returns:     filedata, size of file and response status
def get_filedata_size_and_status(file_name):
    if(".." in file_name or file_name=="/" or file_name=="/index.html"):
        data = build_dir("").encode()
        return data,len(data),"200 OK"
    
    file_name = "./Upload" + file_name
    if(not os.access(file_name,os.F_OK)):                               # Check if the file exists
        data = "<html><head><title>404 Not Found</title></head><body><h1>ERROR 404: Looks like my cat knocked down the file you were requesting for</h1><br><img src=\"sad_cat.png\"></body></html>"
        return data.encode(),len(data.encode()),"404 Not Found"
    if(isgroupreadable(file_name)):                                     # Check for file permissions
        filename, file_extension = os.path.splitext(file_name)          # Check for the file extension
        if(file_extension==".png" or file_extension==".jpg" or file_extension==".jpeg" or file_extension==".gif"):
            try:
                opened_file =  open(file_name, 'rb')
            except:
                if(TEST_FLAG): print(bcolors.FAIL,"OOPS",bcolors.ENDC)
                data = "<html><head><title>Internal Error</title></head><body><h1>Aww Snap...Something wrong in server</h1><br><img src=\"sad_kerby.png\"></body></html>"
                return data.encode(),len(data.encode()),"500"           # Return Internl Error if something wrong
            data = opened_file.read()
            opened_file.close()
            return data,len(data),"200 OK"                              # Everything GOOD!
        elif(file_extension==".txt" or (file_extension=="" and filename!="./Upload/")):
            try:
                opened_file =  open(file_name, 'r')
            except:
                if(TEST_FLAG): print(bcolors.FAIL,"OOPS",bcolors.ENDC)
                data = "<html><head><title>Internal Error</title></head><body><h1>Aww Snap...Something wrong in server</h1><br><img src=\"sad_kerby.png\"></body></html>"
                return data.encode(),len(data.encode()),"500"           # Return Internl Error if something wrong
            data = opened_file.read().encode()                              
            opened_file.close()
            return data,len(data),"200 OK"                              # Everything GOOD!
        else:
            data = build_dir("").encode()                               # Fallback option
            return data,len(data),"200 OK"
    else:
        data = "<html><head><title>Permission Denied</title></head><body><h1>ERROR 401: Permission Denied</h1><br><img src=\"you_shall_not_pass.gif\"></body></html>"
        return data.encode(),len(data.encode()),"401 Unauthorized"

# NOTE _6
# Discription: Parses the http requests made by clients
# Parameters:  Requests
# Returns:     filepath, http method and connection status
def parse_req(req):
    fields = req.split("\r\n")
    url = fields[0].split(" ")[1]
    method = fields[0].split(" ")[0]
    connection = "keep-alive"
    for f in fields:
        if "close" in f:
            connection = "close"
            break
    return url,method,connection

# NOTE _7
# Discription: Class to initialize and start threads
# Parameters:  ClientAddress, ClientSocket and thread number
# Returns:     None
class ClientThread(threading.Thread):
    def __init__(self,clientAddress,clientsocket,thread_no):
        threading.Thread.__init__(self)
        self.csocket = clientsocket
        self.thread_n = thread_no
        if(TEST_FLAG): print (bcolors.OKGREEN,"New connection added: ", clientAddress," at thread_no:",self.thread_n,bcolors.ENDC)
    def run(self):
        if(TEST_FLAG): print (bcolors.WARNING,"Connection from : ", clientAddress,bcolors.ENDC)
        self.csocket.settimeout(15)                                                 # set the timer to 15 seconds for no requests from client
        while True:
            try:
                rd = self.csocket.recv(5000).decode()                               # get request from client
                if(rd==''): break
                url,method,connection = parse_req(rd)                               # note_6
                if(TEST_FLAG): print(bcolors.OKBLUE,"url: ",url,bcolors.ENDC)
                file_data,data_size,htp_status = get_filedata_size_and_status(url)  # note_5
                mimet = mimetypes.guess_type(url)[0]                                # get Content-Type header for the file
                if(mimet==None or htp_status=="404 Not Found" or htp_status=="500" or htp_status=="401 Unauthorized"): mimet = 'text/html'
                
                data = "HTTP/1.1 " + htp_status + "\r\nContent-Type:" + mimet + "\r\n"
                if(connection=="close"): data+= "Connection: Closed\r\n"
                end = "\r\n"
                end = end.encode()
                body_len = data_size + len(end)
                data += "Content-Length: " + str(body_len)                           # finish forming the Response Header  
                start_flag = "\r\n\r\n"
                self.csocket.sendall(data.encode())
                self.csocket.sendall(start_flag.encode())
                if(method=="GET"):
                    self.csocket.sendall(file_data)
                self.csocket.sendall(end)                                           # finished sending Headers, Body and Ending flag
                if(connection=="close"): break                                      # Close the connection if required
            except BaseException as error:
                if(TEST_FLAG): print(bcolors.WARNING,error,bcolors.ENDC)
                break
        if(TEST_FLAG): print (bcolors.FAIL,"Client at ", clientAddress , " disconnected... at thread_no:",self.thread_n,bcolors.ENDC)
        self.csocket.close()

# NOTE _
# Discription: Start program
# Parameters:  None
# Returns:     None
if __name__ == "__main__":
    port = sys.argv[1]                                      # get port number from user
    serversocket = socket(AF_INET, SOCK_STREAM)             # Initialize the socket
    serversocket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)    # Enable reuse of port number
    serversocket.bind(('', int(port)))                      # Bind socket to port number
    serversocket.listen(1)                                  # Start the socket to listen for clients

    if(TEST_FLAG): print(bcolors.OKGREEN + "Server started" + bcolors.ENDC)
    if(TEST_FLAG): print(bcolors.UNDERLINE + "Waiting for client request.." + bcolors.ENDC)
    i=0                                                                     # Thread number
    while(True):
        if(TEST_FLAG): print("active_thread_count: ",threading.active_count())
        clientsock, clientAddress = serversocket.accept()                   # Connect to client
        newthread = ClientThread(clientAddress, clientsock,i)               # Spawn a thread for client
        newthread.start()
        i=i+1
