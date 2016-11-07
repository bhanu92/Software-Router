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

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"

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
uint16_t in_checksum(uint16_t* addr, int count);
//static uint16_t checksum_compute(uint16_t *dat, size_t dat_len);
void sr_route_packet(struct sr_instance* sr, uint8_t* packet, int len, char* interface);
void send_default_route(uint8_t *packet, struct sr_instance *sr, int len);
void print_interface(struct sr_rt *rt);
void sr_send_icmp(
    struct sr_instance* sr,
    uint8_t* packet,
    unsigned int len,
    uint8_t type, uint8_t code);


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
};

struct ip_queue_node *front = NULL;
struct ip_queue_node *rear = NULL;

void enqueue(uint8_t *pack, struct sr_instance* sr, int len) {
	printf("Adding IP packet to queue\n");
	int i;
	i = get_queue_length();
	if (i < 5) {
		struct ip_queue_node *temp = (struct ip_queue_node *)malloc(sizeof(struct ip_queue_node));
		if (temp != NULL) {
			temp->pkt = pack;
			temp->sr_inst = sr;
			temp->length = len;
			temp->next = NULL;
			if ( front == NULL && rear == NULL) {
				front = temp;
				rear = temp;
				return;
			}
			rear->next = temp;
			rear = temp;
		}
	}

	else {

		//dequeue();
		printf("Deleted the entire IP Cache sending the ICMP Unreachable message\n");

		//sr_send_icmp(sr,pack, len, 3, 1);


	}



}

void makeicmp(
    struct sr_icmp_hdr * icmpHdr,
    uint8_t type,
    uint8_t code,
    int len)
{
	icmpHdr->icmp_type = type;
	icmpHdr->icmp_code = code;
	//icmpHdr->icmp_ident = htons(ident);
	//icmpHdr->icmp_seq = htons(seq);

	icmpHdr->icmp_chksum = 0x0000;
	icmpHdr->icmp_chksum = in_checksum((uint16_t*)icmpHdr, len);
}

void makeip(
    struct ip* ipHdr,
    unsigned int len,
    uint16_t off,
    unsigned char ttl,
    unsigned char proto,
    uint32_t src,
    uint32_t dst )
{
	uint32_t sbuf, dbuf;
	uint8_t* p = (uint8_t*) ipHdr;
	*p = 0x45;                      // set ip version and header length
	ipHdr->ip_tos = 0;
	ipHdr->ip_len = htons(len);
	ipHdr->ip_id = 0;               // what should this be?
	ipHdr->ip_off = htons(off);
	ipHdr->ip_ttl = ttl;
	ipHdr->ip_p = proto;

	// read into buffers in case we are overwriting in place
	sbuf = src;
	dbuf = dst;
	ipHdr->ip_src.s_addr = sbuf;
	ipHdr->ip_dst.s_addr = dbuf;

	ipHdr->ip_sum = 0x0000;
	ipHdr->ip_sum = in_checksum((uint16_t*)ipHdr, 20);
}


void sr_send_cachedpacket( uint32_t ipPacket) {

	struct ip_queue_node* reqPacket = front;
	uint8_t* temp_pkt;
	struct sr_rt* req_iface;
	struct ip* ipHdr;
	if (reqPacket) {
		temp_pkt = reqPacket->pkt;
		printf("Send Cached packet\n");
		req_iface = get_interface_from_rt(ipHdr->ip_dst.s_addr, reqPacket->sr_inst);
	}
	if (temp_pkt)
		ipHdr = (struct ip*)(temp_pkt + sizeof(struct sr_ethernet_hdr));


	printf("Got interface\n");
	while (reqPacket != NULL) {


		if (ipHdr->ip_dst.s_addr == ipPacket)
			break;
		reqPacket = reqPacket->next;
	}

	printf("Got packet from queue\n");


	//print_interface(req_iface);
	if (req_iface == NULL)
	{
		printf("Going to default route\n");
		if (reqPacket)
			send_default_route(reqPacket->pkt, reqPacket->sr_inst, reqPacket->length);
	}
	else
	{
		printf("\n coming to this place\n");
		if (reqPacket)
			sr_route_packet(reqPacket->sr_inst, reqPacket->pkt, reqPacket->length, req_iface->interface);
	}
	printf("Route packet successful\n");
}

void print_interface(struct sr_rt *rt)
{
	printf("Interface name - %s\n", rt->interface);

	//printf("IP address  %s\n", inet_ntoa(rt->dest));;
}

void dequeue() {
	struct ip_queue_node *temp = front;
	if (front == NULL)
		return;
	else
	{
		front = NULL;
		rear = NULL;

	}
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
	printf("Caching node\n");
	bool existsInCache = check_arp_cache(arpHdr->ar_sip);
	printf("Caching node\n");
	if (!existsInCache)
	{
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

	if (ntohs(ethHdr->ether_type) == ETHERTYPE_ARP) {
		printf(" \n ether dhost %s\n", ether_ntoa(ethHdr->ether_dhost));
		struct sr_arphdr *arpHdr = (struct sr_arphdr*)(packet + sizeof(struct sr_ethernet_hdr));

		if (arpHdr->ar_op == ntohs(ARP_REQUEST)) {

			struct in_addr dstIP, srcIP;
			dstIP.s_addr = arpHdr->ar_tip;
			srcIP.s_addr = arpHdr->ar_sip;
			printf("\nReceived ARP request from source IP address: %s", inet_ntoa(srcIP));
			/*printf("\nReceived destination IP address: %s", inet_ntoa(dstIP));
			Debug("\nTarget Hardware address\n");
			DebugMAC(ethHdr->ether_dhost);*/
			Debug("\nsender Hardware address\n");
			DebugMAC(arpHdr->ar_sha);
			arp_cache(arpHdr);
			printf("\n");
			//print_arp_cache();
			sr_arp_reply(sr, packet, len, interface); // Function to reply for the arp request
		}

		else if (arpHdr->ar_op == ntohs(ARP_REPLY)) {
			struct in_addr dstIP, srcIP;
			dstIP.s_addr = arpHdr->ar_tip;
			srcIP.s_addr = arpHdr->ar_sip;
			printf("\nReceived ARP reply from the server\n");
			printf( " The MAC address of the server: ");
			DebugMAC(ethHdr->ether_shost);
			printf("\n");
			arp_cache(arpHdr);
			//print_arp_cache();
			sr_send_cachedpacket(arpHdr->ar_sip);

		}

	}
	else if (ntohs(ethHdr->ether_type) == ETHERTYPE_IP) {
		//char intf[6];
		//struct in_addr ipot, ipos;

		struct ip *ipHdr = (struct ip*)(packet + sizeof(struct sr_ethernet_hdr));

		if (ipHdr->ip_p == 1) {
			struct sr_icmp_hdr *icmpHdr = (struct sr_icmp_hdr *)(packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip));
			if (icmpHdr->icmp_type == 0) {
				printf("ECHO reply\n");
			}
			printf("Sender address of IP packet %s\n", inet_ntoa(ipHdr->ip_src));
			printf("Destination address of IP packet %s\n", inet_ntoa(ipHdr->ip_dst));
		}

		struct sr_if* interfs = sr->if_list;

		if (ipHdr->ip_ttl <= 1) {
			printf("Traceroute Packet arrived at router with TTl<=1");
			sr_send_icmp(sr, packet, len , 11, 0);
		}

		while (interfs) {
			if (interfs->ip == ipHdr->ip_dst.s_addr) {
				if (ipHdr->ip_p != 1) {
					sr_send_icmp(sr, packet, len, 3, 3);
					return;
				}
				icmpHdr->icmp_type = 0;
				icmpHdr->icmp_code = 0;

				struct in_addr temp1;
				temp1.s_addr = ipHdr->ip_src.s_addr;
				ipHdr->ip_src = ipHdr->ip_dst;
				ipHdr->ip_dst = temp1;


				icmpHdr->icmp_chksum = 0;
				icmpHdr->icmp_chksum = in_checksum((uint16_t*)icmpHdr, 64);

				send_default_route(packet, sr, len);
				return;


			}
			interfs = interfs->next;
		}

		intf = get_interface_from_rt(ipHdr->ip_dst.s_addr, sr);
		//sr_arp_request(sr, intf->interface, ipHdr->ip_dst.s_addr);
		if (intf != NULL) {
			sr_route_packet(sr, packet, len, intf->interface);
		}
		else {
			send_default_route(packet, sr, len);
		}

		//}

		//}

	}
}/* end sr_ForwardPacket */

/*void sr_send_icmp(uint8_t* packet, struct sr_instance* sr, int len, uint8_t type, uint8_t code) {

	struct sr_ethernet_hdr* ethHdr = (struct sr_ethernet_hdr*)packet;
	struct ip* ipHdr = (struct ip*)(packet + sizeof(struct sr_ethernet_hdr));

	uint8_t* icmpPacket;

	struct sr_ethernet_hdr* icmpETH = ((struct sr_ethernet_hdr*)(malloc(sizeof(struct sr_ethernet_hdr))));
	struct ip* icmpIP = ((struct ip*)(malloc(sizeof(struct ip))));

	if (type == ICMP_DESTINATION_UNREACHABLE) {
		if (code == ICMP_HOST_UNREACHABLE) {
			printf("\nSending ICMP host unreachable message");
		}
		else if (code == ICMP_PORT_UNREACHABLE) {
			printf("\nSending ICMP Port unreachable message");
		}
	}
	else if ((type == ICMP_TIME_EXCEEDED_TYPE) & (code == ICMP_TIME_EXCEEDED_CODE)) {
		printf("\nSending time exceeded packet");
	}

	struct sr_icmp_hdr* icmpHdr = (struct sr_icmp_hdr*)(packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip));
	struct sr_icmp_hdr* icmpICMP = ((struct sr_icmp_hdr*)(malloc(sizeof(struct sr_icmp_hdr))));

	int i;


	memcpy(icmpETH->ether_dhost, ethHdr->ether_shost , sizeof(icmpETH->ether_dhost));
	memcpy(icmpETH->ether_shost, ethHdr->ether_dhost, sizeof(icmpETH->ether_shost));
	printf("ICMP dhost %s\n", ether_ntoa(icmpETH->ether_dhost));
	printf("ICMP shost %s\n", ether_ntoa(icmpETH->ether_shost));

	icmpETH->ether_type = htons(ETHERTYPE_IP);
	icmpIP->ip_v = 4;
	icmpIP->ip_hl = ipHdr->ip_hl;
	icmpIP->ip_tos = ipHdr->ip_tos;
	icmpIP->ip_len = htons((2 * sizeof(struct ip) + sizeof(struct sr_icmp_hdr)) + 8);
	icmpIP->ip_id = ipHdr->ip_id;
	icmpIP->ip_off = ipHdr->ip_off;
	icmpIP->ip_ttl = 64;
	icmpIP->ip_p = 1;
	icmpIP->ip_sum = 0;

	icmpIP->ip_dst.s_addr = ipHdr->ip_src.s_addr;
	struct sr_if* intf = sr->if_list;

	while (intf) {
		if (strcmp(intf->name, "eth0") == 0 ) {
			icmpIP->ip_src.s_addr = intf->ip;
			break;
		}
	}

	printf("ICMP source address %s\n", inet_ntoa(icmpIP->ip_src) );
	printf("ICMP sdestination address %s\n", inet_ntoa(icmpIP->ip_dst) );

	//icmpIP->ip_dst = ipHdr->ip_src;

	icmpIP->ip_sum = in_checksum((uint16_t*)icmpIP, sizeof(struct ip));

	icmpICMP->icmp_type = type;
	icmpICMP->icmp_code = code;
	icmpICMP->icmp_chksum = 0;

	icmpPacket = (uint8_t*)malloc(sizeof(struct sr_ethernet_hdr) + ( 2* sizeof(struct ip)) + sizeof(struct sr_icmp_hdr) + 8);

	memcpy(icmpPacket, icmpETH, sizeof(struct sr_ethernet_hdr));
	memcpy(icmpPacket + sizeof(struct sr_ethernet_hdr) , icmpIP, sizeof(struct ip));
	memcpy(icmpPacket + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip), icmpICMP, sizeof(struct sr_icmp_hdr));
	memcpy(icmpPacket + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct sr_icmp_hdr), ipHdr, sizeof(struct ip));

	for(i = 0 ; i<8;i++){
		icmpPacket[sizeof(struct sr_ethernet_hdr) + (2*sizeof(struct ip))+sizeof(struct sr_icmp_hdr)+i] =
																				packet[sizeof(struct sr_ethernet_hdr)+sizeof(struct ip)+i];
	}

	//icmpICMP->icmp_chksum = in_checksum(icmpPacket + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip),
	 //                                   sizeof(struct sr_icmp_hdr) + sizeof(struct ip) + 8);


	icmpICMP->icmp_chksum = in_checksum((uint16_t*)icmpICMP, 64);
	sr_send_packet(sr, icmpPacket, sizeof(struct sr_ethernet_hdr) + (2*sizeof(struct ip))+ sizeof(struct sr_icmp_hdr) + 8, "eth0" );

	free(icmpPacket);
	free(icmpETH);
	free(icmpIP);
	free(icmpICMP);
}*/

void sr_send_icmp(struct sr_instance* sr, uint8_t* packet, unsigned int len, uint8_t type, uint8_t code) {

	/* allocate memory for our new packet */
	uint8_t* icmpPacket = malloc(70 * sizeof(uint8_t));
	if (icmpPacket == NULL) {
		fprintf(stderr, "Error: malloc could not find memory for packet storage\n");
		return;
	}
	memset(icmpPacket, 0, 70 * sizeof(uint8_t));

	/* organize our src packet */
	struct sr_ethernet_hdr* srcethernetHdr = (struct sr_ethernet_hdr*)packet;
	struct ip* srcipHdr = (struct ip*)(packet + 14);

	/* organize pointers for our new packet */
	struct sr_ethernet_hdr* newEthHdr = (struct sr_ethernet_hdr*)icmpPacket;
	struct ip* newipHdr = (struct ip*)(icmpPacket + 14);
	struct icmp_hdr* newicmpHdr = (struct icmp_hdr*)(icmpPacket + 34);
	uint8_t* newicmpData = (uint8_t*)(icmpPacket + 42);

	/* copy src ip header + tcp/udp ports to icmp data */
	memcpy(newicmpData, srcipHdr, 28);

	/* create icmp, ip and ethernet headers on our new packet */
	makeicmp(newicmpHdr, type, code, 36);
	makeip(newipHdr, 70 - 14, IP_DF, 64, IPPROTO_ICMP,
	       sr_get_interface(sr, "eth0")->ip, srcipHdr->ip_src.s_addr);
	memcpy(newEthHdr->ether_dhost, srcethernetHdr->ether_shost , sizeof(newEthHdr->ether_dhost));
	memcpy(newEthHdr->ether_shost, srcethernetHdr->ether_dhost, sizeof(newEthHdr->ether_shost));
	printf("ICMP dhost %s\n", ether_ntoa(newEthHdr->ether_dhost));
	printf("ICMP shost %s\n", ether_ntoa(newEthHdr->ether_shost));

	newEthHdr->ether_type = htons(ETHERTYPE_IP);

	/* send away */
	sr_send_packet(sr, icmpPacket, 70, "eth0");

// log on send
	if (type == ICMP_PORT_UNREACHABLE)
		printf("<-- ICMP Destination Port Unreachable sent to %s\n", inet_ntoa(newipHdr->ip_dst));
	if (type == ICMP_HOST_UNREACHABLE)
		printf("<-- ICMP Destination Host Unreachable sent to %s\n", inet_ntoa(newipHdr->ip_dst));
	if (type == ICMP_TIME_EXCEEDED_TYPE)
		printf("<-- ICMP Time Exceeded message sent to %s\n", inet_ntoa(newipHdr->ip_dst));

	free(icmpPacket);
}


void sr_route_packet(struct sr_instance* sr, uint8_t* packet, int len, char* interface) {

	struct sr_ethernet_hdr* ethHdr = (struct sr_ethernet_hdr*)packet;
	struct ip *ipHdr = (struct ip*)(packet + sizeof(struct sr_ethernet_hdr));
	unsigned char mac_addr[6];

	struct sr_icmp_hdr *icmpHdr = (struct sr_icmp_hdr*)(packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip));


	if (check_arp_cache(ipHdr->ip_dst.s_addr) == true) {
		//  mac_addr = get_mac_addr_cache(ipHdr->ip_dst.s_addr);
		int i;
		if (ipHdr->ip_p == 1) {
			printf("ICMP packet\n");

			if (icmpHdr->icmp_type == 8) {
				printf("ECHO request\n");

				icmpHdr->icmp_chksum = 0;
				icmpHdr->icmp_chksum = in_checksum((uint16_t*)icmpHdr, 64);
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
		DebugMAC(ethHdr->ether_dhost);
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
		ipHdr->ip_sum = in_checksum((uint16_t*)ipHdr, 20);
		//ipHdr->ip_sum=checksum_compute((uint16_t*) ipHdr, 32);
		sr_send_packet(sr, packet, len, interface);
	}

	else {
		enqueue(packet, sr, len);
		sr_arp_request(sr, packet, interface, ipHdr->ip_dst.s_addr, len);
	}

}


uint16_t in_checksum(uint16_t* addr, int count)
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


void sr_arp_reply(struct sr_instance* sr, uint8_t * packet, unsigned int len, char* interface) {

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
	printf("\n\nSending ARP reply to the browser \n\n");

	/*printf("sender IP address %s\n" , inet_ntoa(sip));
	printf("Destination IP address %s\n", inet_ntoa(tip));
	printf("Hardware Address of sender ");
	DebugMAC(arpReply->ar_sha);
	printf("\n");
	printf("Hardware address of destination ");
	DebugMAC(arpReply->ar_tha); */
	sr_send_packet(sr, packet, len, interface); // calling the send packet function from sr_router.h
}
static int arp_request_count = 0;
void sr_arp_request(struct sr_instance* sr, uint8_t* pkt, char* interface, uint32_t destIP, int length) {

	arp_request_count++;

	if (arp_request_count > 5) {
		dequeue();
		sr_send_icmp(sr, pkt, length , 3, 1);
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
				//ethHdr->ether_shost[j] = req_iface->addr[j];
				arpHdr->ar_sha[i] = req_iface->addr[i];
			}
			//printf("\nRouter Hardware address in ARP header");
			//DebugMAC(arpHdr->ar_sha);
			memcpy(ethHdr->ether_shost, req_iface->addr, sizeof(req_iface->addr));
			//memcpy(arpHdr->ar_sha, req_iface->addr, sizeof(req_iface->addr));
			//printf("\nRouter Hardware address in ethernet header");
			//DebugMAC(ethHdr->ether_shost);
			arpHdr->ar_sip = req_iface->ip;
			struct in_addr temp;
			//temp.s_addr = arpHdr->ar_sip;
			//printf("\nRouter IP adress %s", inet_ntoa(temp));
			temp.s_addr = arpHdr->ar_tip;
			printf("\nSending ARP request to the Server %s from interface %s\n", inet_ntoa(temp), interface);
			sr_send_packet(sr, (uint8_t*)ethHdr, len, req_iface->name);
		}
	}

}


struct sr_rt* get_interface_from_rt(uint32_t dest, struct sr_instance* sr) {
	struct sr_rt *routing_table = sr->routing_table;
	struct in_addr temp;
	temp.s_addr = dest;
	//printf("Actual dest is %s\n", inet_ntoa(temp));
	while (routing_table)
	{
		if ((routing_table->dest).s_addr != 0)
		{
			uint32_t dest_table = (routing_table->dest).s_addr;
			uint32_t mask = (routing_table->mask).s_addr;
			uint32_t prefix = dest & mask;

			temp.s_addr = prefix;

			//printf("Result of AND is %s\n", inet_ntoa(temp));
			temp.s_addr = dest;
			//printf("Searching for %s\n", inet_ntoa(temp));
			if (dest_table == prefix)
			{
				//printf("\n\nMatch\n\n");
				return routing_table;


			}
		}
		routing_table = routing_table->next;
	}


	return NULL;
}

void send_default_route(uint8_t *packet, struct sr_instance *sr, int len) {
	struct sr_rt *routing_table = sr->routing_table;
	struct sr_ethernet_hdr *ethHdr = (struct sr_ethernet_hdr *)packet;
	struct ip *ipHdr = (struct ip *)(packet + sizeof(struct sr_ethernet_hdr));
	char interface[5];
	//struct sr_icmp_hdr icmpHdr =  (struct sr_icmp_hdr*)(packet+sizeof(struct sr_ethernet_hdr)+sizeof(struct ip));
	printf("\n\n\nGoing with default route\n\n\n");
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
	ipHdr->ip_sum = in_checksum((uint16_t*)ipHdr, 20);
	//ipHdr->ip_sum=checksum_compute((uint16_t*) ipHdr, 32);
	printf("Destination gateway MAC address %s\n", ether_ntoa(ethHdr->ether_dhost));
	printf("Source gateway MAC address %s\n", ether_ntoa(ethHdr->ether_shost));
	sr_send_packet(sr, packet, len, interface);



	//printf("\nSending to default route...\n");
	//sr_send_packet(sr,packet,len,interface);
}




/*---------------------------------------------------------------------
 * Method:
 *
 *---------------------------------------------------------------------*/