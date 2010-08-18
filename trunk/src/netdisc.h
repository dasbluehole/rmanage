#ifndef _NET_DISCOVER_H_
#define _NET_DISCOVER_H_
#define CONF_FILE "/etc/rmanage/rmanage.cnf"
/* protocol application combo */
typedef struct myapp{
		char *protocol;
		int port;
		char *app;
		struct myapp *next;
	}application;
/*interface or nic */
typedef struct mydevice{
		int if_number;
		char name[16];
		char ip[16];
		char mask[16];
		char broadcast[16];
		char mac[20];
		int configured;
		struct mydevice *next;
	}device;
/* individual node(machine) device */
struct net_node
		{
			char ip[16];
			char mac[20];
			char hostname[1004]; //BIND-9.X defines this.RFC 1123
			int alive;
			//int compat; //dont need this to be in the node list, rather it will be detected on the fly from a dialog.
			struct net_node *next;
		};
typedef struct net_node netnode;
char current_nic_dev[16];
void get_my_ip(char *ip);
device *my_get_configured_device_list(device *list);
void my_get_device_conf(device *dev);
device *my_get_device_list(void);
unsigned long get_network_address(char *ip,char *mask,char *net_address);
char *num_to_ip(unsigned long netnum);
netnode *my_make_netnode_list(char *network_ip,char *broadcast_ip);
void my_discover_compat_node_list(netnode *list,int myport);
int create_raw_socket_for_protocol(char *protocol);
static int in_cksum(unsigned short *addr, int len);
int pinger(char *dest_ip,int mseq);
int is_port_open(char *ip,int port);
application *get_configured_application_list(char *filename);
//higher level functions
device *make_eth_dev_list();
netnode *make_nodes_list(device *eth_dev);
char *get_remote_host_name(char *ip);

#endif

