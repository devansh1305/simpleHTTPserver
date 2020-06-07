# Project Title

Simple HTTP Server

## Getting Started

Start the server and then the client

### Prerequisites

Python 3.6.8

### Running the server

```
python3 ./myserver.py [port]
```

After executing the server, it will print the incoming connections and also file requests.

### Running the client

```
python3 ./myclient.py [serverIP] [serverPort]
```
>**Example Commands:**
```
GET / HTTP/1.1
GET /pic1.png HTTP/1.1
GET pic1.png HTTP/1.1
HEAD /pic3.png HTTP/1.1
```

## Running the tests

Test for persistant connection:

In myclient.py file, Uncomment lines #36,#42,#46,#49,#56 and Comment lines #35,#42,#47,#48,#55,#86 to automatically test for persistent connection using a series of requests

You can see that all the files are requested without any disruptions in connection as no disconnect statements are printed on the server end.

## Authors

**Devansh Panirwala**
