/**********************************************************************
 * file:  sr_router.c
 * date:  Mon Feb 18 12:50:42 PST 2002
 * Contact: casado@stanford.edu
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing. 11
 * 90904102
 **********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/time.h>

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_pwospf.h"

#define MAX_QUEUE_SIZE 5
#define ICMP_DESTINATION_UNREACHABLE 3
#define ICMP_HOST_UNREACHABLE 1
#define ICMP_PORT_UNREACHABLE 3
#define ICMP_TIME_EXCEEDED_TYPE 11
#define ICMP_TIME_EXCEEDED_CODE 0


/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
	/* REQUIRES */
	assert(sr);

	/* Add initialization code here! */

} /* -- sr_init -- */

struct sr_rt* get_interface_from_rt(uint32_t dest, struct sr_instance* sr);
void sr_arp_request(struct sr_instance* sr, uint8_t* packet, char* interface, uint32_t destIP, int len);
void sr_arp_reply(struct sr_instance* sr, uint8_t* packet, unsigned int len, char* interface);
bool validate_ip_packet(struct ip *ipHdr);
uint16_t packet_checksum(uint16_t* addr, int count);
void sr_route_packet(struct sr_instance* sr, uint8_t* packet, int len, char* interface);
void send_default_route(uint8_t *packet, struct sr_instance *sr, int len);
void print_interface(struct sr_rt *rt);
void sr_send_icmp(struct sr_instance* sr, uint8_t* packet, unsigned int len, uint8_t type, uint8_t code);
void dequeue();
unsigned char* get_mac_addr_cache(uint32_t ipAddr);


struct sr_icmp_hdr
{
	uint8_t icmp_type;
	uint8_t icmp_code;
	uint16_t icmp_chksum;
	uint16_t icmp_ident;
	uint16_t icmp_seq;
};

struct ip_queue_node
{
	uint8_t *pkt;
	struct sr_instance* sr_inst;
	int length;
	struct ip_queue_node *next;
	bool sent;
};


struct ip_queue_node ip_cache[5];
struct ip_queue_node *rear = NULL,*front=NULL;

void enqueue(uint8_t *pack, struct sr_instance* sr, int len) {
	assert(pack);
	assert(sr);
	printf("Adding IP packet to queue\n");
	int i;
	for(i=0;i<5;i++)
		if(ip_cache[i].pkt==NULL)
		{
			ip_cache[i].pkt = pack;
			ip_cache[i].sr_inst = sr;
			ip_cache[i].sent=false;
			ip_cache[i].length = len;
			break;
		}
	
}

void new_icmp(struct sr_icmp_hdr * icmpHdr, uint8_t type, uint8_t code, int len)
{
	icmpHdr->icmp_type = type;
	icmpHdr->icmp_code = code;
	//icmpHdr->icmp_ident = htons(ident);
	//icmpHdr->icmp_seq = htons(seq);

	icmpHdr->icmp_chksum = 0;
	icmpHdr->icmp_chksum = packet_checksum((uint16_t*)icmpHdr, len);
}

void new_ip(struct ip* ipHdr, unsigned int len, uint16_t off, unsigned char ttl, unsigned char proto, uint32_t src, uint32_t dst )
{
	uint32_t temp1, temp2;
	uint8_t* p = (uint8_t*) ipHdr;
	*p = 0x45;                      // set ip version and header length
	ipHdr->ip_tos = 0;
	ipHdr->ip_len = htons(len);
	ipHdr->ip_id = 0;               // what should this be?
	ipHdr->ip_off = htons(off);
	ipHdr->ip_ttl = ttl;
	ipHdr->ip_p = proto;
	temp1 = src;
	temp2 = dst;
	ipHdr->ip_src.s_addr = temp1;
	ipHdr->ip_dst.s_addr = temp2;

	ipHdr->ip_sum = 0;
	ipHdr->ip_sum =  packet_checksum((uint16_t*)ipHdr, 20);
}


void sr_send_cachedpacket( uint32_t ipPacket) {

	assert(ipPacket);
	//struct ip_queue_node* reqPacket = front;
	uint8_t* temp_pkt;
	struct sr_rt* req_iface;
	struct ip* ipHdr;
	uint32_t longestMatch = 0;
	int i;

	
	
	for(i=0;i<5;i++)
	{
		
		if(ip_cache[i].pkt!=NULL)
		{
			ipHdr = (struct ip *)(ip_cache[i].pkt + sizeof(struct sr_ethernet_hdr));
			req_iface = get_interface_from_rt(ipHdr->ip_dst.s_addr, ip_cache[i].sr_inst);
		
			if(req_iface==NULL)
			{
				printf("Sending through gateway\n");
				struct sr_ethernet_hdr *ethHdr = (struct sr_ethernet_hdr *)(ip_cache[i].pkt);
				struct sr_if *iflist = ip_cache[i].sr_inst->if_list;
				int i;
				while(iflist)
				{
					if(strcmp(iflist->name,"eth0")==0)
						break;
					iflist=iflist->next;
				}
				for(i=0;i<6;i++)
					ethHdr->ether_shost[i]=iflist->addr[i];
					
				struct sr_rt *routing_table = ip_cache[i].sr_inst->routing_table;
				while(routing_table)
				{
					if(routing_table->dest.s_addr==0 && strcmp(routing_table->interface,"eth0"))
						break;
					routing_table=routing_table->next;
				}
				char *addr = get_mac_addr_cache(routing_table->gw.s_addr);
				for(i=0;i<6;i++)
					ethHdr->ether_dhost[i]=addr[i];
				sr_send_packet(ip_cache[i].sr_inst,ip_cache[i].pkt,ip_cache[i].length,"eth0");
				
			}
			else
			{
			//printf("Got packet from queue\n");
				printf("Got last packet from queue and sending through %s\n", req_iface->interface); 
				sr_route_packet(ip_cache[i].sr_inst, ip_cache[i].pkt, ip_cache[i].length, req_iface->interface);
							
			}
			
		
		}
		
	}
	//dequeue();
	
}

void print_interface(struct sr_rt *rt)
{
	printf("Interface name - %s\n", rt->interface);

	//printf("IP address  %s\n", inet_ntoa(rt->dest));;
}

void dequeue() {
	
	struct ip_queue_node *temp = front;
	if (front == NULL)
		printf("IP cache queue is empty\n");
		return;
	if(front == rear){
		front = NULL;
		rear =NULL;
	}
	struct ip_queue_node *temp2;
	while(temp)
	{
		if(temp->sent==true)
		{
			temp2=temp->next;
			free(temp);
			temp=temp2;
		}
		temp=temp->next;
	}
	front=temp2;
}


int get_queue_length() {
	struct ip_queue_node *temp = front;
	int i;
	i = 1;

	while (temp) {
		i++;
		temp = temp->next;
	}
	return i;
}

struct cache_node *head_cache = NULL;
//struct cache_node *end_cache = NULL;

struct cache_node
{
	uint32_t ip_addr;
	unsigned char hw_addr[6];
	struct cache_node *next;
};

bool check_arp_cache(uint32_t ipAddr)
{
	struct cache_node *temp = head_cache;
	while (temp) {
		if (temp->ip_addr == ipAddr)
		{
			return true;
		}
		temp = temp->next;
	}
	return false;
}

unsigned char* get_mac_addr_cache(uint32_t ipAddr) {
	struct cache_node *temp = head_cache;
	while (temp) {
		if (temp->ip_addr == ipAddr)
		{
			return temp->hw_addr;
		}
		temp = temp->next;
	}
	return NULL;
}


void arp_cache(struct sr_arphdr* arpHdr ) {

	bool existsInCache = check_arp_cache(arpHdr->ar_sip);
	if (!existsInCache)
	{
		printf("Caching MAC Address\n");
		struct cache_node* temp = (struct cache_node*)malloc(sizeof(struct cache_node));
		struct cache_node* temp1 = head_cache;
		int i;
		if (temp != NULL) {
			temp->ip_addr = arpHdr->ar_sip;
			//  unsigned char* temp_hw= (unsigned char*)malloc(6*sizeof(unsigned char));
			//  temp_hw=arpHdr->ar_sha;
			// temp->hw_addr = temp_hw;
			for (i = 0; i < ETHER_ADDR_LEN; i++)
			{
				temp->hw_addr[i] = arpHdr->ar_sha[i];
			}
			temp->next = NULL;
			printf("Caching the new MAC Address: %s\n",ether_ntoa(arpHdr->ar_sha));
			if (head_cache == NULL) {
				head_cache = temp;
				return;
			}

			while (temp1->next != NULL)
			{
				temp1 = temp1->next;
			}
			temp1->next = temp;

			//printf("breakpoint\n");
		}
	}

	else
	{
		printf("Already exists in cache\n");
	}
}

void print_arp_cache() {
	struct cache_node *temp = head_cache;
	int i = 0;
	struct in_addr iptemp;
	while (temp) {
		iptemp.s_addr = temp->ip_addr;

		printf("Node %d: %s\t", i, inet_ntoa(iptemp));
		DebugMAC(temp->hw_addr);
		printf("\n");
		temp = temp->next;
		i++;
	}
}
void destroy() {
	struct cache_node *temp = head_cache;

	while (temp)
	{
		struct cache_node *nxt = temp->next;
		free(temp);
		temp = nxt;
	}

}

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
                     uint8_t * packet/* lent */,
                     unsigned int len,
                     char* interface/* lent */)
{
	/* REQUIRES */
	assert(sr);
	assert(packet);
	assert(interface);
	struct sr_rt* intf;


	printf("\n*** -> Received packet of length %d \n", len);
	struct sr_ethernet_hdr *ethHdr = (struct sr_ethernet_hdr*)packet;
	struct sr_if* iface = sr_get_interface(sr, interface);

	if (ntohs(ethHdr->ether_type) == ETHERTYPE_ARP) {
		printf(" \n ether dhost %s\n", ether_ntoa(ethHdr->ether_dhost));
		struct sr_arphdr *arpHdr = (struct sr_arphdr*)(packet + sizeof(struct sr_ethernet_hdr));

		if (arpHdr->ar_op == ntohs(ARP_REQUEST)) {

			struct in_addr dstIP, srcIP;
			dstIP.s_addr = arpHdr->ar_tip;
			srcIP.s_addr = arpHdr->ar_sip;
			printf("******************************************\n");
			printf("Received ARP request from source IP address: %s\n", inet_ntoa(srcIP));
			Debug("Sender MAC address: ");
			DebugMAC(arpHdr->ar_sha);
			printf("\n");
			printf("******************************************\n");
			arp_cache(arpHdr);
			//print_arp_cache();
			sr_arp_reply(sr, packet, len, interface); // Function to reply for the arp request
		}

		else if (arpHdr->ar_op == ntohs(ARP_REPLY)) {
			struct in_addr dstIP, srcIP;
			dstIP.s_addr = arpHdr->ar_tip;
			srcIP.s_addr = arpHdr->ar_sip;
			printf("******************************************\n");
			printf("Received ARP reply from the server\n");
			printf( "The MAC address of the server: ");
			DebugMAC(ethHdr->ether_shost);
			printf("\n");
			printf("******************************************\n");
			arp_cache(arpHdr);
			//print_arp_cache();
			sr_send_cachedpacket(arpHdr->ar_sip);

		}

	}
	else if (ntohs(ethHdr->ether_type) == ETHERTYPE_IP) {
		//char intf[6];
		//struct in_addr ipot, ipos;

		struct ip *ipHdr = (struct ip*)(packet + sizeof(struct sr_ethernet_hdr));
		struct sr_if* interfs = sr->if_list;
		printf("******************************************\n");
		printf("Sender address of IP packet %s\n", inet_ntoa(ipHdr->ip_src));
		printf("Destination address of IP packet %s\n", inet_ntoa(ipHdr->ip_dst));
		printf("******************************************\n");
		if (ipHdr->ip_p == 89) {

			//handle OSPF packets
			printf("Handling OSPF packet\n");
			handle_ospf_packet(sr, packet, iface);
			return;
		}
		if (ipHdr->ip_p == 1) {
			struct sr_icmp_hdr *icmpHdr = (struct sr_icmp_hdr *)(packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip));
			if (icmpHdr->icmp_type == 0) {
				printf("******************************************\n");
				printf("ECHO reply\n");
				printf("Sender address of IP packet %s\n", inet_ntoa(ipHdr->ip_src));
				printf("Destination address of IP packet %s\n", inet_ntoa(ipHdr->ip_dst));
				printf("******************************************\n");
			}
			else if ((icmpHdr->icmp_type == 3) && (icmpHdr->icmp_code == 3) ) {
				printf("\nForwarding the ICMP time exceeded msg received frm server\n");
				send_default_route(packet, sr, len);
				return;
			}

			while (interfs) {
				if (interfs->ip == ipHdr->ip_dst.s_addr) {

					icmpHdr->icmp_type = 0;
					icmpHdr->icmp_code = 0;

					struct in_addr temp1;
					temp1.s_addr = ipHdr->ip_src.s_addr;
					ipHdr->ip_src = ipHdr->ip_dst;
					ipHdr->ip_dst = temp1;


					icmpHdr->icmp_chksum = 0;
					icmpHdr->icmp_chksum =  packet_checksum((uint16_t*)icmpHdr, 64);

					send_default_route(packet, sr, len);
					return;


				}
				interfs = interfs->next;
			}
		}

		if (ipHdr->ip_p == 6 || ipHdr->ip_p == 17) {
			while (interfs) {
				if (interfs->ip == ipHdr->ip_dst.s_addr) {
					printf("UDP or TCP Packet to the router");
					sr_send_icmp(sr, packet, len, 3, 3);
					return;
				}

				interfs = interfs->next;
			}
		}

		if (ipHdr->ip_ttl <= 1) {
			printf("\nReceived message with TTL<=1\n");
			sr_send_icmp(sr, packet, len, 11, 0);
			return;
		}



		intf = get_interface_from_rt(ipHdr->ip_dst.s_addr, sr);
		//sr_arp_request(sr, intf->interface, ipHdr->ip_dst.s_addr);
		if (intf != NULL) {
			sr_route_packet(sr, packet, len, intf->interface);
		}
		else {
			send_default_route(packet, sr, len);
		}

	}
}/* end sr_ForwardPacket */


void sr_send_icmp(struct sr_instance * sr, uint8_t* packet, unsigned int len, uint8_t type, uint8_t code) {

	assert(sr);
	assert(packet);

	/* allocate memory for our new packet */
	uint8_t* icmpPacket = malloc(70 * sizeof(uint8_t));
	if (icmpPacket == NULL) {
		printf("malloc storage error\n");
		return;
	}
	memset(icmpPacket, 0, 70 * sizeof(uint8_t));

	struct sr_ethernet_hdr* recv_etherHdr = (struct sr_ethernet_hdr*)packet;
	struct ip* recv_ipHdr = (struct ip*)(packet + 14);

	struct sr_ethernet_hdr* send_ethHdr = (struct sr_ethernet_hdr*)icmpPacket;
	struct ip* send_ipHdr = (struct ip*)(icmpPacket + 14);
	struct icmp_hdr* send_icmpHdr = (struct icmp_hdr*)(icmpPacket + 34);
	uint8_t* icmp_data = (uint8_t*)(icmpPacket + 42);

	send_ethHdr->ether_type = htons(ETHERTYPE_IP);

	memcpy(icmp_data, recv_ipHdr, 28);
	new_icmp(send_icmpHdr, type, code, 36);
	new_ip(send_ipHdr, 70 - 14, IP_DF, 64, IPPROTO_ICMP, sr_get_interface(sr, "eth0")->ip, recv_ipHdr->ip_src.s_addr);
	memcpy(send_ethHdr->ether_shost, recv_etherHdr->ether_dhost, sizeof(send_ethHdr->ether_shost));
	memcpy(send_ethHdr->ether_dhost, recv_etherHdr->ether_shost , sizeof(send_ethHdr->ether_dhost));

	sr_send_packet(sr, icmpPacket, 70, "eth0");

	printf("******************************************\n");
	printf("Destination MAC address of ICMP packet %s\n", ether_ntoa(send_ethHdr->ether_dhost));
	printf("Source MAC address of ICMP packet%s\n", ether_ntoa(send_ethHdr->ether_shost));
	printf("Creating and Forwarding the ICMP packet\n");
	printf("******************************************\n");

	free(icmpPacket);
}

void sr_route_packet(struct sr_instance * sr, uint8_t* packet, int len, char* interface) {

	assert(sr);
	assert(packet);
	assert(interface);
	printf("Entered Route Packet method\n");
	struct sr_ethernet_hdr* ethHdr = (struct sr_ethernet_hdr*)packet;
	struct ip *ipHdr = (struct ip*)(packet + sizeof(struct sr_ethernet_hdr));
	unsigned char mac_addr[6];
	struct sr_rt *rTable = get_interface_from_rt(ipHdr->ip_dst.s_addr, sr);
	if (check_arp_cache(ipHdr->ip_dst.s_addr) == true)
	{

		int i;
		if (ipHdr->ip_p == 1) {
			struct sr_icmp_hdr *icmpHdr = (struct sr_icmp_hdr*)(packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip));
			printf("ICMP packet\n");

			if (icmpHdr->icmp_type == 8) {
				printf("ECHO request\n");

				icmpHdr->icmp_chksum = 0;
				icmpHdr->icmp_chksum = packet_checksum((uint16_t*)icmpHdr, 64);
			}
			else if (icmpHdr->icmp_type == 0) {
				printf("ECHO reply\n");
			}
			else if (icmpHdr->icmp_type == 11) {
				printf("Time exceeded message\n");
			}

		}
		if (ipHdr->ip_p == 17) {

			printf("UDP packet for traceroute\n");
			//printf("TTL is %d\n", ipHdr->ip_ttl);
			if (ipHdr->ip_ttl == 0) {
				sr_send_icmp(sr, packet, len, ICMP_TIME_EXCEEDED_TYPE, 0);
				return;
			}


		}

		struct cache_node *temp = head_cache;
		while (temp) {
			if (temp->ip_addr == ipHdr->ip_dst.s_addr)
			{
				for (i = 0; i < 6; i++)
					mac_addr[i] = temp->hw_addr[i];
				break;
			}
			temp = temp->next;
		}
		memcpy(ethHdr->ether_dhost, mac_addr, sizeof(ethHdr->ether_dhost));
		DebugMAC(mac_addr);
		printf("\n");
		//DebugMAC(ethHdr->ether_dhost);
		struct sr_if* req_iface = sr->if_list;
		while (req_iface)
		{
			if (strcmp(req_iface->name, interface) == 0)
			{
				break;
			}

			req_iface = req_iface->next;
		}

		for (i = 0; i < ETHER_ADDR_LEN; i++) {
			ethHdr->ether_shost[i] = req_iface->addr[i];
			//arpHdr->ar_sha[i] = req_iface->addr[i];
		}

		if ( ipHdr->ip_v != 4)
			return;

		if (ipHdr->ip_ttl == 0) return ; // write code for send an ICMP bessage back
		else
			ipHdr->ip_ttl = ipHdr->ip_ttl - 1;
		ipHdr->ip_sum = 0;
		ipHdr->ip_sum = packet_checksum((uint16_t*)ipHdr, 20);
		printf("Length of packet to be sent is\n", len);
		//ipHdr->ip_sum=checksum_compute((uint16_t*) ipHdr, 32);
		sr_send_packet(sr, packet, len, interface);
	}
	else if (rTable && rTable->dest.s_addr != 0 && check_arp_cache(rTable->gw.s_addr) == true) {
		//  mac_addr = get_mac_addr_cache(ipHdr->ip_dst.s_addr);
		int i;
		if (ipHdr->ip_p == 1) {
			struct sr_icmp_hdr *icmpHdr = (struct sr_icmp_hdr*)(packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip));
			printf("ICMP packet\n");

			if (icmpHdr->icmp_type == 8) {
				printf("ECHO request\n");

				icmpHdr->icmp_chksum = 0;
				icmpHdr->icmp_chksum = packet_checksum((uint16_t*)icmpHdr, 64);
			}
			else if (icmpHdr->icmp_type == 0) {
				printf("ECHO reply\n");
			}
			else if (icmpHdr->icmp_type == 11) {
				printf("Time exceeded message\n");
			}

		}
		if (ipHdr->ip_p == 17) {

			printf("UDP packet for traceroute\n");
			//printf("TTL is %d\n", ipHdr->ip_ttl);
			if (ipHdr->ip_ttl == 0) {
				sr_send_icmp(sr, packet, len, ICMP_TIME_EXCEEDED_TYPE, 0);
				return;
			}


		}

		struct cache_node *temp = head_cache;
		while (temp) {
			if (temp->ip_addr == rTable->gw.s_addr)
			{
				for (i = 0; i < 6; i++)
					mac_addr[i] = temp->hw_addr[i];
				break;
			}
			temp = temp->next;
		}
		memcpy(ethHdr->ether_dhost, mac_addr, sizeof(ethHdr->ether_dhost));
		DebugMAC(mac_addr);
		printf("\n");
		struct sr_if* req_iface = sr->if_list;
		while (req_iface)
		{
			if (strcmp(req_iface->name, interface) == 0)
			{
				break;
			}

			req_iface = req_iface->next;
		}

		for (i = 0; i < ETHER_ADDR_LEN; i++) {
			ethHdr->ether_shost[i] = req_iface->addr[i];
			//arpHdr->ar_sha[i] = req_iface->addr[i];
		}

		if ( ipHdr->ip_v != 4)
			return;

		if (ipHdr->ip_ttl == 0) return ; // write code for send an ICMP bessage back
		else
			ipHdr->ip_ttl = ipHdr->ip_ttl - 1;
		ipHdr->ip_sum = 0;
		ipHdr->ip_sum = packet_checksum((uint16_t*)ipHdr, 20);
		printf("Length of packet to be sent is %d\n\n", len);
		//ipHdr->ip_sum=checksum_compute((uint16_t*) ipHdr, 32);
		sr_send_packet(sr, packet, len, interface);
	}

	else {
		enqueue(packet, sr, len);
		struct sr_rt *rtable = get_interface_from_rt(ipHdr->ip_dst.s_addr, sr);
		if (rtable->gw.s_addr != 0){
			printf("Destination for ARP packet :%s\n",inet_ntoa(rtable->gw));
			sr_arp_request(sr, packet, rtable->interface, rtable->gw.s_addr, len);
		}
		else{
			printf("Destination for ARP packet :%s\n",inet_ntoa(ipHdr->ip_dst));
			sr_arp_request(sr, packet, rtable->interface, ipHdr->ip_dst.s_addr, len);
		}
	}

}


uint16_t packet_checksum(uint16_t* addr, int count)
{
	register uint32_t sum = 0;

	while (count > 1) {
		sum += *addr++;
		count -= 2;
	}

	if (count > 0)
		sum += *((uint8_t*)addr);

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return (~sum);
}


void sr_arp_reply(struct sr_instance * sr, uint8_t * packet, unsigned int len, char* interface) {

	assert(sr);
	assert(packet);
	assert(interface);

	uint32_t temp_ip;

	struct sr_ethernet_hdr *ethHdr = (struct sr_ethernet_hdr*)packet;
	struct sr_arphdr *arpReply = (struct sr_arphdr*)(packet + sizeof(struct sr_ethernet_hdr));
	struct sr_if *if_sender = sr_get_interface(sr, interface);

	memcpy(ethHdr->ether_dhost, ethHdr->ether_shost, sizeof(ethHdr->ether_dhost)); // setting destination mac address
	memcpy(ethHdr->ether_shost, if_sender->addr, sizeof(ethHdr->ether_shost));     // setting source mac address

	ethHdr->ether_type = htons(ETHERTYPE_ARP); //setting the type in ethernet header
	arpReply->ar_hrd = htons(ARPHDR_ETHER); //setting the type of physical network
	arpReply->ar_pro = htons(ETHERTYPE_IP); //setting the type of protocol
	arpReply->ar_hln = 6;     //setting length of MAC address
	arpReply->ar_pln = 4;    //setting length of IP address
	arpReply->ar_op = htons(ARP_REPLY); // setting op-code

	memcpy(arpReply->ar_tha, arpReply->ar_sha, sizeof(arpReply->ar_tha)); //setting target mac address in arp header
	memcpy(arpReply->ar_sha, if_sender->addr, sizeof(arpReply->ar_sha)); //settig source mac address in arp header
	//memcpy(arpReply->arp_sip, arpReply->arp_tip, sizeof(arpReply->arp_sip));

	temp_ip = arpReply->ar_sip;
	arpReply->ar_sip = arpReply->ar_tip;  // setting source IP address in arp header
	arpReply->ar_tip = temp_ip;    //setting target IP address in ARP header.

	struct in_addr sip, tip;
	sip.s_addr = arpReply->ar_sip;
	tip.s_addr = arpReply->ar_tip;
	printf("******************************************\n");
	printf("Sending ARP reply to the browser\n");
	printf("Src IP: %s\t",inet_ntoa(sip));
	printf("Dst IP: %s\n", inet_ntoa(tip));
	printf("Src MAC: %s\t",ether_ntoa(arpReply->ar_sha));
	printf("Dst MAC: %s\n",ether_ntoa(arpReply->ar_tha));
	printf("Sending ARP reply from Interface %s\n",interface);
	printf("******************************************\n");

	sr_send_packet(sr, packet, len, interface); // calling the send packet function from sr_router.h
}


static int arp_request_count = 0;
void sr_arp_request(struct sr_instance * sr, uint8_t* pkt, char* interface, uint32_t destIP, int length) {

	assert(sr);
	assert(pkt);
	assert(interface);

	arp_request_count++;
	struct ip* temp_ip = (struct ip*)(pkt + 14);

	if (arp_request_count > 5 && temp_ip != NULL && temp_ip->ip_p != 17 ) {
		dequeue();
		sr_send_icmp(sr, pkt, length , 3, 1);
		arp_request_count = 0;
		return;
	}
	bool exists;
	exists = check_arp_cache(destIP);
	int len = sizeof( struct sr_ethernet_hdr) + sizeof(struct sr_arphdr);
	uint8_t* packet =  malloc (len);
	if (packet != NULL) {
		struct sr_ethernet_hdr* ethHdr = (struct sr_ethernet_hdr*)packet;
		struct sr_arphdr* arpHdr = (struct sr_arphdr*)(packet + sizeof(struct sr_ethernet_hdr));
		arpHdr->ar_hrd = ntohs(1);
		arpHdr->ar_pro = ntohs(ETHERTYPE_IP);
		arpHdr->ar_op = ntohs(ARP_REQUEST);
		arpHdr->ar_hln = 6;
		arpHdr->ar_pln = 4;
		arpHdr->ar_tip = destIP;
		ethHdr->ether_type = ntohs(ETHERTYPE_ARP);
		if (!exists)
		{


			int i;

			for (i = 0; i < 6; i++) {
				ethHdr->ether_dhost[i] = 255;
			}

			struct sr_if* req_iface = sr->if_list;
			
			while (req_iface)
			{
				if (strcmp(req_iface->name, interface) == 0)
				{
					break;
				}
				req_iface = req_iface->next;
			}

			for (i = 0; i < ETHER_ADDR_LEN; i++) {

				arpHdr->ar_sha[i] = req_iface->addr[i];
			}

			memcpy(ethHdr->ether_shost, req_iface->addr, sizeof(req_iface->addr));
			arpHdr->ar_sip = req_iface->ip;
			struct in_addr temp;
			temp.s_addr = arpHdr->ar_tip;
			printf("\nSending ARP request to the Server %s from interface %s\n", inet_ntoa(temp), interface);
			sr_send_packet(sr, (uint8_t*)ethHdr, len, req_iface->name);
		}
	}

}


struct sr_rt* get_interface_from_rt(uint32_t dest, struct sr_instance * sr) {

	
	struct sr_rt *routing_table = sr->routing_table;
	struct in_addr temp;
	temp.s_addr = dest;
	uint32_t max = 0;
	struct sr_rt* maxRt;
	int numMatches=0;
	//sr_print_routing_table(sr);
	//printf("Actual dest is %s\n", inet_ntoa(temp));
	while (routing_table)
	{
		
		uint32_t res1 = dest & routing_table->mask.s_addr;
		uint32_t res2 = (routing_table->dest.s_addr)&(routing_table->mask.s_addr);
		if(res1==res2 && routing_table->dest.s_addr!=0)
		{
			return routing_table;
			//printf("Match occurred\n");
			//numMatches++;
		}
		//printf("AND operation is %s\n",ip);
		


		routing_table = routing_table->next;
	}
	if(numMatches>=1)
	{
		routing_table=sr->routing_table;
		while(routing_table)
		{
			uint32_t res1 = dest & routing_table->mask.s_addr;
			uint32_t res2 = (routing_table->dest.s_addr)&(routing_table->mask.s_addr);
			if(res1==res2)
			{
				printf("Interface is %s\n",routing_table->interface);
				return routing_table->interface;
			}
			routing_table = routing_table->next;
		}
			
	}
	return NULL;
	
}

void send_default_route(uint8_t *packet, struct sr_instance * sr, int len) {
	struct sr_rt *routing_table = sr->routing_table;
	struct sr_ethernet_hdr *ethHdr = (struct sr_ethernet_hdr *)packet;
	struct ip *ipHdr = (struct ip *)(packet + sizeof(struct sr_ethernet_hdr));
	char interface[5];
	//struct sr_icmp_hdr icmpHdr =  (struct sr_icmp_hdr*)(packet+sizeof(struct sr_ethernet_hdr)+sizeof(struct ip));
	printf("\nGoing with default route\n");
	//print_arp_cache();

	while (routing_table)
	{
		if (strcmp(inet_ntoa(routing_table->dest), "0.0.0.0") == 0)
		{
			//ipHdr->ip_dst = routing_table->gw;
			//printf("\ngateway ip address%s\n",inet_ntoa(ipHdr->ip_dst));
			int i;
			for (i = 0; i < 4; i++)
				interface[i] = routing_table->interface[i];
			interface[4] = '\0';
			break;
		}
		routing_table = routing_table->next;
	}
	if (check_arp_cache(routing_table->gw.s_addr) == true)
	{
		unsigned char mac_addr[6];
		struct cache_node *temp = head_cache;
		int i;
		while (temp) {
			if (temp->ip_addr == routing_table->gw.s_addr)
			{
				//memcpy(ethHdr->ether_dhost, temp->hw_addr, sizeof(temp->hw_addr));
				//memcpy(ethHdr->ether_dhost, htons("00:50:56:94:27:c8"), sizeof(temp->hw_addr));
				for (i = 0; i < 6; i++) {
					ethHdr->ether_dhost[i] = temp->hw_addr[i];
					//arpHdr->ar_sha[i] = req_iface->addr[i];
				}
				break;
			}
			temp = temp->next;
		}
	}
	else
	{
		enqueue(packet, sr, len);
		sr_arp_request(sr, packet, interface, routing_table->gw.s_addr, len);
	}

	struct sr_if* req_iface = sr->if_list;
	int i;
	while (req_iface)
	{

		if (strcmp(req_iface->name, interface) == 0)
		{
			break;
		}

		req_iface = req_iface->next;
	}

	for (i = 0; i < 6; i++) {
		ethHdr->ether_shost[i] = req_iface->addr[i];
		//arpHdr->ar_sha[i] = req_iface->addr[i];
	}

	if ( ipHdr->ip_v != 4)
		return;

	if (ipHdr->ip_ttl == 0) return ; // write code for send an ICMP bessage back
	else
		ipHdr->ip_ttl = ipHdr->ip_ttl - 1;
	ipHdr->ip_sum = 0;
	ipHdr->ip_sum = packet_checksum((uint16_t*)ipHdr, 20);
	//ipHdr->ip_sum=checksum_compute((uint16_t*) ipHdr, 32);
	printf("******************************************\n");
	printf("Destination gateway MAC address %s\n", ether_ntoa(ethHdr->ether_dhost));
	printf("Source gateway MAC address %s\n", ether_ntoa(ethHdr->ether_shost));
	printf("******************************************\n");
	sr_send_packet(sr, packet, len, interface);



	//printf("\nSending to default route...\n");
	//sr_send_packet(sr,packet,len,interface);
}




/*---------------------------------------------------------------------
 * Method:
 *
 *---------------------------------------------------------------------*/