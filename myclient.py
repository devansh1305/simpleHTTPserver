# python3 myclient.py [serverIP] [serverPort]
import socket
import os
import sys

# NOTE _0 Uncomment lines #36,#42,#46,#49,#56 and Comment lines #35,#42,#47,#48,#55,#86 to test for persistent connection

# NOTE _1
# Discription: parse response header to get content length
# Parameters:  response header
# Returns:     Content-Length
def parse_head(content_header):
    arr = content_header.split("\n")
    for i in arr:
        if("Content-Length" in i):
            lent = int(''.join(filter(str.isdigit, i)))
            return lent

# NOTE _2
# Discription: Check if the file is an image
# Parameters:  filename and file-extension
# Returns:     Boolean True/False
def isImage(filename, file_extension):
    if(file_extension==".png" or file_extension==".jpg" or file_extension==".jpeg" or file_extension==".gif"): return True
    return False

# NOTE _3
# Discription: Start program
# Parameters:  None
# Returns:     None
if __name__ == "__main__":
    SERVER = sys.argv[1]                                            # Get server name from user
    PORT = int(sys.argv[2])                                         # Get server port from user

    send_req = input("Enter request to continue or Enter 'exit' to quit: ")
    #send_req = ["GET / HTTP/1.1" ,"GET /pic1.png HTTP/1.1", "GET /pic2.png HTTP/1.1", "GET /pic3.png HTTP/1.1", "GET /pic4.png HTTP/1.1", "GET /pic5.png HTTP/1.1", "GET /nyan.gif HTTP/1.1", "GET /sad_cat.png HTTP/1.1", "GET /sad_kerby.png HTTP/1.1", "GET /sampletext.txt HTTP/1.1", "GET /you_shall_not_pass.gif HTTP/1.1"] # note_0

    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)      # Initialize Client socket
    client.connect((SERVER, PORT))                                  # Connect to the server

    i=0                                                             # number of requests made
    #while(i<len(send_req)):                                         # note_0
    while(send_req!="exit"):                                       
        method,start_flag,content_header,file_cont= "","\r\n\r\n","",""   # Initialize flags
        img_cont = "".encode()
        #print("request:",send_req[i])                               # note_0
        print("request:",send_req)
        method, file_name, stat = send_req.split(" ")              # Get http method, name of file and status code
        #method, file_name, stat = send_req[i].split(" ")            # note_0

        if(file_name[0]!="/"): file_name = "/" + file_name          # parse for better file handeling
        if(file_name=="/"): file_name="/index.html"
        is_img = isImage(os.path.splitext(file_name)[0],os.path.splitext(file_name)[1])
        
        client.sendall(send_req.encode())                          # send the request to server
        #client.sendall(send_req[i].encode())                        # note_0
        
        while True:
            content_header = content_header + client.recv(1).decode()
            if(start_flag in content_header): break
        print("Content Header: ",content_header)                    # Get the Response Header
        
        if(method=="GET"):                                          # Get the Response Body
            cnt_lent = parse_head(content_header)                   # note_1
            got_data=0
            get_buf = 1024
            while (got_data<=cnt_lent):                             # Get the body using the content length (decode if necessary)
                if(not is_img): 
                    file_buf =  client.recv(get_buf)
                    file_cont += file_buf.decode() 
                    got_data+=len(file_buf)
                else: 
                    file_buf =  client.recv(get_buf)
                    img_cont += file_buf 
                    got_data+=len(file_buf)
                if(get_buf==0): break
                if(get_buf>=(cnt_lent-got_data)): get_buf = cnt_lent-got_data
            file_dir = "./Download" + file_name
            if(not is_img): 
                file_f = open(file_dir,"w")
                file_f.write(file_cont)                            # Write body to Download directory
            else: 
                img_f = open(file_dir,"wb")
                img_f.write(img_cont)                              # Write body to Download directory
        
        send_req = input("Enter request to continue or Enter 'exit' to quit: ")
        i+=1
    client.close()                                                 # Disconnect from server