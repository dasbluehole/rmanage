#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<errno.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/ip_icmp.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<features.h>
#include<linux/if_packet.h>
#include<linux/if_ether.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<net/if_arp.h>
#define icmphdr icmp
#define iphdr ip
#include "netdisc.h"

//int my_port=22;
fd_set fd;

/* this function takes a sockaddr structure and returns the MAC id as string*/
static char *ethernet_mactoa(struct sockaddr *addr) 
{ 
	static char buff[256]; 
	unsigned char *ptr = (unsigned char *) addr->sa_data;

	sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", 
		(ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377), 
		(ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377)); 

return (buff); 

}
char *ip_to_mac(char *ip)
{
	int s; //socket
	static char mac[20];
	struct arpreq areq;
	struct sockaddr_in *sin;
	struct in_addr ipaddr;
	s=socket(AF_INET,SOCK_DGRAM,0);
	if(s==-1) //error
	{
		printf("unable to create a socket\n");
		return NULL;
	}
	memset(&areq,0,sizeof(areq));
	sin = (struct sockaddr_in *)&areq.arp_pa;
	sin->sin_family = AF_INET;
	if(inet_aton(ip,&ipaddr)==0)
	{
		printf("invalid IP address %s\n",ip);
		if(s)
			close(s);
		return NULL;
	}
	sin->sin_addr=ipaddr;
	sin = (struct sockaddr_in *) &areq.arp_ha;
	sin->sin_family = ARPHRD_ETHER;
	strncpy(areq.arp_dev, current_nic_dev, 15); //current_nic_dev is defined elsewhere
	if (ioctl(s, SIOCGARP, (caddr_t) &areq) == -1)
		strcpy(mac,"Not Available");
	else
		strcpy(mac,ethernet_mactoa(&areq.arp_ha));
	if(s)
		close(s);
	//printf("%s -> %s \n",ip,mac);
	return mac;
}
void populate_mac_id(netnode *list)
{
	netnode *temp;
	temp=list;
	while(temp)
	{
		if(temp->alive)
		{
			//printf("Checking %s ->",temp->ip);
			strcpy(temp->mac,ip_to_mac(temp->ip));
			//printf("%s\n",temp->mac);
		}
		temp=temp->next;
	}
}
device *my_get_device_list(void)
{
	device *list=NULL, *temp=NULL, *temp1=NULL;
	char dev_name[20];
	struct if_nameindex *dev_arr,*temp_dev; // the array will be returned by if_nameindex()
	dev_arr=if_nameindex();
	if(dev_arr==NULL) // if dev_arr is null then if_nameindex() failed ENOBUF
        {
                perror("if_nameindex:");
                exit(0);
        }
	temp_dev=dev_arr;
	// now we got all the devices in dev_arr
	 while(dev_arr->if_index)
        {
                printf("%d %s\n",dev_arr->if_index, dev_arr->if_name);
		temp=malloc(sizeof(device)*sizeof(char));
		if(temp==NULL)
			return NULL;
		bzero(temp,sizeof(device));
		temp->if_number=dev_arr->if_index;
		strcpy(temp->name,dev_arr->if_name);
		if(list==NULL) 
			list=temp;
		else
		{
			temp->next=list;
			list=temp;
		}
		dev_arr++;
        }
	if_freenameindex(temp_dev); //now free the resource allocated by if_nameindex()
	temp=list;
	while(temp)
	{
		my_get_device_conf(temp);
		temp=temp->next;
	}
	return list;
}

void my_get_device_conf(device *dev)
{
	struct ifreq ifr;
	struct ifreq *IFR;	
	int loopback=0;	
	struct sockaddr_in saddr;
	int sock, i;
	unsigned char macid[6];	
	
	//decorate our device
	
	strcpy(dev->ip,"-NA-");
	strcpy(dev->mask,"-NA-");
	strcpy(dev->broadcast,"-NA-");
	strcpy(dev->mac,"-NA-");
	strcpy(ifr.ifr_name, dev->name);
	//strcpy(dev->name,IFR->ifr_name);
	sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock ==-1)
		return;
	if(ioctl(sock,SIOCGIFADDR,&ifr)==0)
	{
		saddr=*((struct sockaddr_in *)(&(ifr.ifr_addr)));
		strcpy(dev->ip,inet_ntoa(saddr.sin_addr));
	}
	if(ioctl(sock,SIOCGIFNETMASK,&ifr)==0)
	{
		saddr=*((struct sockaddr_in *)(&(ifr.ifr_addr)));
		strcpy(dev->mask,inet_ntoa(saddr.sin_addr));
	}
	if(ioctl(sock,SIOCGIFBRDADDR,&ifr)==0)
	{
		saddr=*((struct sockaddr_in *)(&(ifr.ifr_broadaddr)));
		strcpy(dev->broadcast,inet_ntoa(saddr.sin_addr));
	}
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
	{
		if(ifr.ifr_flags & IFF_LOOPBACK)
		{
			strcpy(dev->mac,"LOOPBACK");
			
		}
		else if(ioctl(sock,SIOCGIFHWADDR,&ifr)==0)
		{
			saddr=*((struct sockaddr_in *)(&(ifr.ifr_hwaddr)));
			memcpy(macid,&ifr.ifr_hwaddr.sa_data,6);	
			sprintf(dev->mac,"%2X:%2x:%2x:%2X:%2x:%2x",macid[0],macid[1],macid[2],macid[3],macid[4],macid[5]);
		}
	}
	close(sock);
}
device *my_get_configured_device_list(device *list)
{
	struct ifreq ifr;
	struct ifreq *IFR;
	struct ifconf ifc;
	struct sockaddr_in saddr;
	device *dev=NULL,*temp=NULL;
	char buf[1024];
	int sock, i;
	unsigned char macid[6];
	//make a socket
	sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock ==-1)
		return NULL;
	//fill up the config structure	
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	//do ioctl for device config
	ioctl(sock, SIOCGIFCONF, &ifc);
	IFR = ifc.ifc_req;
	for (i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; IFR++)
	{
		int loopback=0;		
		//decorate our device
		dev=(device *)malloc(sizeof(struct mydevice));
		if(dev==NULL)
			return list;
		bzero(dev,sizeof(device));
		strcpy(dev->ip,"-NA-");
		strcpy(dev->mask,"-NA-");
		strcpy(dev->broadcast,"-NA-");
		strcpy(dev->mac,"-NA-");
		strcpy(ifr.ifr_name, IFR->ifr_name);
		strcpy(dev->name,IFR->ifr_name);
		if(ioctl(sock,SIOCGIFADDR,&ifr)==0)
		{
			saddr=*((struct sockaddr_in *)(&(ifr.ifr_addr)));
			strcpy(dev->ip,inet_ntoa(saddr.sin_addr));
		}
		if(ioctl(sock,SIOCGIFNETMASK,&ifr)==0)
		{
			saddr=*((struct sockaddr_in *)(&(ifr.ifr_addr)));
			strcpy(dev->mask,inet_ntoa(saddr.sin_addr));
		}
		if(ioctl(sock,SIOCGIFBRDADDR,&ifr)==0)
		{
			saddr=*((struct sockaddr_in *)(&(ifr.ifr_broadaddr)));
			strcpy(dev->broadcast,inet_ntoa(saddr.sin_addr));
		}
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
		{
			if(ifr.ifr_flags & IFF_LOOPBACK)
			{
				strcpy(dev->mac,"LOOPBACK");
				continue;
			}
			if(ioctl(sock,SIOCGIFHWADDR,&ifr)==0)
			{
				saddr=*((struct sockaddr_in *)(&(ifr.ifr_hwaddr)));
				memcpy(macid,&ifr.ifr_hwaddr.sa_data,6);	
				sprintf(dev->mac,"%2X:%2x:%2x:%2X:%2x:%2x",macid[0],macid[1],macid[2],macid[3],macid[4],macid[5]);
			}
		}
		dev->if_number=if_nametoindex(dev->name);
		temp=list;
		while(temp)
		{
			if(strcmp(temp->name,dev->name)==0)
				temp->configured=1;
			temp=temp->next;
		}
			
						
		//dev->next=list;
		//list=dev;
	}//end for
	close(sock);	
	return list;
	
}
unsigned long get_network_address(char *ip,char *mask,char *net_address)
{
	unsigned long ipint=0,maskint=0,netint=0;
	struct sockaddr_in inp;
	bzero(&inp,sizeof(struct sockaddr_in));
	if(inet_aton(ip,&inp.sin_addr)!=0)
		ipint=(int)inp.sin_addr.s_addr;
	//printf("deb   %s\n",inet_ntoa(inp.sin_addr));
	if(inet_aton(mask,&inp.sin_addr)!=0)
		maskint=(int)inp.sin_addr.s_addr;
	//printf("ipint %u maskint %u\n",ipint,maskint);
	netint=ipint&maskint;
	inp.sin_addr.s_addr=netint;
	strcpy(net_address,inet_ntoa(inp.sin_addr));
	return netint;

}
char *num_to_ip(unsigned long netnum)
{
	struct sockaddr_in temp;
	temp.sin_addr.s_addr=netnum;
	return(inet_ntoa(temp.sin_addr));
}

//we have to discover the network by pinging
int create_raw_socket_for_protocol(char *protocol)
{
	struct protoent *proto;
	int sock_raw;
	if (!(proto = getprotobyname(protocol)))
	{
		(void)fprintf(stderr, "unknown protocol %s.\n",protocol);
		exit(2);
	}
	if ((sock_raw = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) 
	{
		if (errno==EPERM) 
		{
			fprintf(stderr, "must run as root\n");
		}
		else perror("socket");
		exit(2);
	}
	return sock_raw;
}
//check-sum calculator for ip packets
static int in_cksum(unsigned short *addr, int len)
{
	register int nleft = len;
	register unsigned short *w = addr;
	register int sum = 0;
	unsigned short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(unsigned char *)(&answer) = *(unsigned char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}
//////first ping code capable of broadcasting.
int pinger(char *dest_ip,int mseq)
{
		
	unsigned char out_packet[4096]; //maximum packet size
	int pid,res,cc,rv,fdmask;
	int hold=48*1024;
	char buff[4096];
	int raw_socket;
	int fromlen;
	int iphdrlen;
	struct sockaddr_in dest_address,from;
	struct iphdr *ip;
	struct timeval *tp,timeout;
	int datalen=(64-8); //default datalen
	//printf("checking %s\n",dest_ip);
	//struct timezone tz;
	timeout.tv_sec=0;
	timeout.tv_usec=5000;
	raw_socket=create_raw_socket_for_protocol("icmp");
	FD_SET(raw_socket, &fd);
	//fill up dest_address structure first.
	fdmask= 1<<raw_socket;
	memset(&dest_address,0,sizeof(struct sockaddr_in));
	dest_address.sin_family=AF_INET;
	dest_address.sin_port=0;
	dest_address.sin_addr.s_addr=inet_addr(dest_ip);
	struct icmp *icp = (struct icmp *) out_packet;
	tp = (struct timeval *) &out_packet[8];//8-byte time
	u_char *datap = &out_packet[8+sizeof(struct timeval)];//data area
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = mseq;
	pid=(int)getpid();
	icp->icmp_id = pid;
	gettimeofday((struct timeval *)&out_packet[8],(struct timezone *)NULL);
	cc=datalen+8;	
	icp->icmp_cksum = in_cksum((unsigned short *)icp, cc);
	setsockopt(raw_socket, SOL_SOCKET, SO_BROADCAST, &hold, sizeof(hold));
	setsockopt(raw_socket, SOL_SOCKET,SO_REUSEADDR,&hold,sizeof(hold));
	res=sendto(raw_socket,(char *)out_packet, cc, 0,(struct sockaddr *)&dest_address,sizeof(struct sockaddr));
	if(res <0 || res != cc)
	{
		if(res== -1)
		{
			printf("Error sending packet\n");
			perror("sendto");
		}
	}
	//printf("ping: wrote %s %d chars, ret=%d\n", dest_ip, cc, res);
	for(;;){	
resend:
		rv = select(raw_socket+1, (fd_set *)&fdmask, NULL, NULL, &timeout);
		if(rv== -1)
		{
			//printf("error select\n");
			close(raw_socket);
			usleep(100);
			return 0;
		}
		if(rv==0)
		{
			//printf("warning: hostdown, send error, or unreachable ....\n");
			close(raw_socket);
			usleep(100);
			return 0;
		} 
	
		if(FD_ISSET(raw_socket,&fd))
		{	
			res=recvfrom(raw_socket,&buff,sizeof(buff),0,(struct sockaddr *)&from,&fromlen);
			if(res==-1)
			{
				//printf("error: recvfrom returned -1\n");
				close(raw_socket);
				usleep(100);				
				return 0;
			}
			//now the buffer contains the ip header + icmp data
			//separate out the ip header part.
			ip=(struct iphdr *)buff;
			iphdrlen=ip->ip_hl<<2;
			//get the begining of ICMP
			icp=(struct icmphdr *)(buff+iphdrlen);
			//Check if it is an echo reply
			if (icp->icmp_type == ICMP_ECHOREPLY)
			{
				if (icp->icmp_id ==pid)
				{
					//our packet
					//printf("*[%s] Alive ",inet_ntoa(from.sin_addr));
					//printf("seq %d \n",icp->icmp_seq);
					close(raw_socket);
					usleep(100);
					return 1;
				}	
			}						
		}
	}//for
	close(raw_socket);
	usleep(100);
	return 0;
}
//this takes network_ip=network address and net_mask=network mask in dotted IP notation
// returns a list of netnode
netnode *my_make_netnode_list(char *network_ip,char *broadcast_ip)
{
	int i;
	unsigned long network,num_nodes,broadcast;
	netnode *list=NULL,*tempnode=NULL;
	//struct sockaddr_in temp;	
	
	//get the number of nodes	
	if((network=inet_network(network_ip))==-1)
		return NULL;
	if((broadcast=inet_network(broadcast_ip))==-1)
		return NULL;
	num_nodes=broadcast-network;
	printf("broadcast= %u network= %u Total nodes = %u \n",broadcast,network,num_nodes);
	for(i=1;i<num_nodes;i++)
	{
		tempnode=(netnode *)malloc(sizeof(struct net_node));
		if(tempnode!=NULL)
		{
			bzero(tempnode,sizeof(netnode));			
			//printf("%s\n",num_to_ip(htonl(network+i)));
			strcpy(tempnode->ip,num_to_ip(htonl(network+i)));
			tempnode->next=list;
			list=tempnode;
		}
	}
	return list;
}
void check_live_node(netnode *list)
{
	netnode *temp;
	temp=list;
	while(temp)
	{
		if(pinger(temp->ip,1)==1)
		{
			//printf("alive %s ",temp->ip);
			temp->alive=1;
		}
		temp=temp->next;
	}
	//printf("\n");
}
void populate_host_name(netnode *list)
{
	netnode *temp;
	temp=list;
	while(temp)
	{
		if(temp->alive)
		{
			strcpy(temp->hostname,get_remote_host_name(temp->ip));
		}
		temp=temp->next;
	}
}
/* This function is not needed to be in the nodeliost.
 rather it should be detected after the node is selected from a dialog.
void my_discover_compat_node_list(netnode *list,int myport)
{
	netnode *temp;
	int sock;
	int menu=1,hold=1;
	struct sockaddr_in peer;	
	temp=list;
	while(temp)
	{
		if(temp->alive==1)
		{		
			peer.sin_family=AF_INET;
			peer.sin_port=htons(myport);
			peer.sin_addr.s_addr=inet_addr(temp->ip);
			sock=socket(AF_INET,SOCK_STREAM,0);
			if(sock==-1)
			{
				printf("Error: could not create socket.\n");
				return;
			}
			setsockopt(sock, SOL_SOCKET,SO_REUSEADDR,&hold,sizeof(hold));
			if(connect(sock, (struct sockaddr *)&peer,sizeof(peer))!=-1)
			{
				temp->compat=menu;
				//printf("compat %s\n",temp->ip);
				menu++;
			}
			close(sock);
			usleep(100);
		}
		temp=temp->next;
	}
}*/
application *get_configured_application_list(char *filename)
{
	FILE *fp;
	char *buff;
	char protoname[20]="",appname[1024]="",*token;
	int portnum=0;
	application *applist=NULL,*temp=NULL;
	fp=fopen(filename,"r");
	if(!fp)
	{
		printf("Error: opening file [%s] for read\n",filename);
		return applist;
	}
 	buff=(char *)malloc(80*sizeof(char));
	if(!buff)
	{
		printf("Error: malloc failed\n");
		return applist;
	}
	while(!feof(fp))
	{	//search for tag [protocol] on a single line
		fgets(buff,80,fp);
		buff[strlen(buff)-1]='\0';
		if((buff[0]=='#') || buff[0]==';') continue; //comments		
		if(strncmp(buff,"[protocol]",10)==0)
		{
			//found the section
			// read repeatedly 
			while(!feof(fp))
			{
				buff=(char *)malloc(80*sizeof(char));
				if(!buff)
				{
					printf("Error: malloc failed\n");
					return applist;
				}
				fgets(buff,80,fp);
				buff[strlen(buff)-1]='\0';
				token=strsep(&buff," \t");
				if((token[0]=='\0') ||(token[0]=='[')) return applist;
				strcpy(protoname,token);
				token=strsep(&buff," \t");
				if((token[0]=='\0') ||(token[0]=='[')) return applist;
				portnum=atoi(token);
				token=strsep(&buff," \t");
				if((token[0]=='\0') ||(token[0]=='[')) return applist;
				strcpy(appname,token);
				temp=(application *)malloc(sizeof(application));
				if(temp==NULL)
				{
					return applist;
				}
				printf("protocol %s port %d application %s\n",protoname,portnum,appname);
				free(buff);
				temp->protocol=malloc(strlen(protoname));				
				strcpy(temp->protocol,protoname);
				temp->app=malloc(strlen(appname));
				strcpy(temp->app,appname);
				temp->port=portnum;
				temp->next=applist;
				applist=temp;	
			}
		}
	}
	return applist;
}
	
	
// this function will return -1 if a perticular port is not oppened else a positive number.
int is_port_open(char *ip,int port)
{
	int sock,port_open=-1;
	struct sockaddr_in peer;
	int hold=1;
	peer.sin_family=AF_INET;
	peer.sin_port=htons(port);
	peer.sin_addr.s_addr=inet_addr(ip);
	sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock==-1)
	{
		printf("Error: could not create socket.\n");
		return;
	}
	setsockopt(sock, SOL_SOCKET,SO_REUSEADDR,&hold,sizeof(hold));
	if(connect(sock, (struct sockaddr *)&peer,sizeof(peer))!=-1)
	{
		port_open=1;
	}
	close(sock);
	usleep(500);
	return port_open;
}

//upper level functions
// make_eth_dev_list() will return a list of all network devices with configured ones marked 
device *make_eth_dev_list()
{
	device *eth_list=NULL;
	eth_list=my_get_device_list();
	eth_list=my_get_configured_device_list(eth_list);
	return eth_list;
}
//make_nodes_list(...) takes one eth_device and port to check; and returns a list of all nodes with alive and opened port 'port'
netnode *make_nodes_list(device *eth_dev)
{
	char net_ip[16];	
	unsigned long netaddn;
	netnode *node_list=NULL;	
	netaddn=get_network_address(eth_dev->ip,eth_dev->mask,net_ip);
	printf("network %s\n",net_ip);
	node_list=my_make_netnode_list(net_ip,eth_dev->broadcast);
	printf("%p\n",node_list);
	check_live_node(node_list);
	populate_host_name(node_list);
	populate_mac_id(node_list);
	//my_discover_compat_node_list(node_list,port); //will be done in the dialog after an IP is selected.
	printf("done \n");
	return node_list;
}
char *get_remote_host_name(char *ip)
{
	static char name[1024]="\0";
	struct sockaddr_in sa;	
	struct hostent *he;
	sa.sin_addr.s_addr=inet_addr(ip);
	he=gethostbyaddr((char *)&sa.sin_addr.s_addr,sizeof(sa.sin_addr.s_addr),AF_INET);
	if(he)
	{
		strcpy(name,he->h_name);
	}
	return name;
}
#undef DEBUG_
