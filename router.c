#include "router.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>

int init_udp(int port){
  int optval;
  struct sockaddr_in udpserveraddr; /* server's addr */
  /* 
   * socket: create the parent socket 
   */
  int udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpsockfd < 0) 
    return 0;

    /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(udpsockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

   /*
   * build the server's Internet address
   */
  bzero((char *) &udpserveraddr, sizeof(udpserveraddr));
  udpserveraddr.sin_family = AF_INET;
  udpserveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  udpserveraddr.sin_port = htons((unsigned short)port);

    /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(udpsockfd, (struct sockaddr *) &udpserveraddr, 
     sizeof(udpserveraddr)) < 0) 
    return 0;

  return udpsockfd;
}

int main(int argc, char** argv){
	int udpsockfd;
  	int myPort;
  	struct hostent *hostp;
  	struct sockaddr_in netAddr;
  	int netLen = sizeof(netAddr);;
  	int n;
  	int myID;
  	char* targetIP;
  	int targetPort;
  	struct hostent *hp;


  	//no enough parameter
  	if(argc != 5){
  		return 0;
  	}

  	myID = atoi(argv[1]);
  	targetIP = argv[2];
  	targetPort = atoi(argv[3]);
  	myPort = atoi(argv[4]);

  	udpsockfd = init_udp(myPort);

  	//set the server 
  	memset(&netAddr, 0, sizeof(netAddr)); 
  	hp = gethostbyname(targetIP);
  	netAddr.sin_family = AF_INET;
  	netAddr.sin_port = htons(targetPort);
  	bcopy((char *)hp->h_addr, (char *)&netAddr.sin_addr.s_addr, hp->h_length);
  	netLen = sizeof(netAddr); 

  	//init router
  	struct pkt_INIT_REQUEST startup;
  	struct pkt_INIT_REQUEST* startup_p = &startup;
  	startup.router_id = myID;
  	ntoh_pkt_INIT_RESPONSE(startup_p);
  	n = sendto(udpsockfd, (char*)startup_p, sizeof(startup), 0, (struct sockaddr *) &netAddr, netLen);
}