# Computer Network Applications
Multi-threaded Networking applications such as a simplified FTP, and reliable TCP-like communication protocol over unreliable links

## UDP & TCP sockets
udp-sockets & tcp-sockets contain the implementation of a client and server which can be used to fetch small files from the server through the respective protocol.

block-based-file transfer contains implementation of a TCP client and server which transfers files in blocks of a fixed size.

asynchronous-io contains a UDP echo server and client which uses a **signal driven** handler for I/O.

## Concurrent Server
A concurrent server which can receive 2 different service requests from the client:
* *Request for a bag of words*, where the server will forward a set of words from a file word.txt one after another over a stream socket. The file word.txt contains a set of words, one each at every line. Once the server receives a request for the words, it reads the words from the file and forwards them one after another, each as a null terminated string. The end of service is marked with an empty string (a string only with a null character). Once the client receives that empty string, it prints the number of words received and exits.
* *Request for the IP address* corresponding to a domain name, where the client requests for the IP address of a domain, say www.google.com, over a datagram socket. The server looks up for the IP address by using the system call gethostbyname(). The server returns this IP address to the client. The client prints the IP address and exits.

## Simplified File Transfer Protocol (FTP)
An iterative FTP server which services the commands:
* **port** *Y*- Y is the port number of the data port at which the client waits (this must be the first command sent to the server)
* **cd** *dir_name*
* **get** *file_name* - fetches file from server (file_name must either be a file or the absolute path to the file)
* **put** *file_name* - sends file from client's local directory to server's current directory
* **quit** - closes connection with client and waits for next client

## Reliable Communication over an Unreliable Link
An API is built for a MRP (My Reliable Protocol) socket, which guarantees that any message sent using a MRP socket is always delivered to the receiver exactly once. However, unlike TCP sockets, MRP sockets may not guarantee in-order delivery of messages. Thus messages may be delivered out of order. UDP is used as the unreliable link. 

For simulation and testing purposes, messages over the unreliable UDP linked are forcefully dropped with *P* probability.

Refer to *documentation.txt* in the folder for more details regarding the multi-threaded implementation.

## Traceroute
A custom version of the Linux traceroute tool for identifying the number of layer 3 (IP layer) hops from your machine to a given destination, which has been built using raw sockets.

## Ping
A custom version of ping utility which can be used to check the connectivity status between a source and a destination over an IP network, using ICMP requests. Includes support for:
* -t ttl
* -i interval
* -c count
* -W timeout
* -s packetsize
* -w deadline

## HTTP Server
A concurrent multithreaded HTTP Server that can handle GET and HEAD requests.
