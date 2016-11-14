/*-----------------------------------------------------------------------------
 * file: sr_pwospf.c
 * date: Tue Nov 23 23:24:18 PST 2004
 * Author: Martin Casado
 *
 * Description:
 *
 *---------------------------------------------------------------------------*/

#include "sr_pwospf.h"
#include "sr_router.h"
#include "pwospf_protocol.h"
#include "sr_protocol.h"
#include "sr_if.h"
#include "sr_topology.h"

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <string.h>

/* -- declaration of main thread function for pwospf subsystem --- */
static void* pwospf_run_thread(void* arg);
void sr_send_lsu(void* arg);
void sr_send_hello(void* arg);
void sr_monitor_neighors(void *arg);
char* get_interface(struct sr_instance *sr, uint32_t ip);
char* get_mac_address(struct sr_instance *sr, char *intf);
uint32_t get_router_ip(struct sr_instance *sr, char *intf);
bool check_interface_in_neighbors(char *intf);
typedef struct ospfv2_lsu lsu_pkt;
//void sr_add_to_advertisements(lsu_pkt *head,uint32_t subnet, uint32_t mask);
typedef struct sr_ethernet_hdr ethernet_hdr;
typedef struct ip ip_hdr;
typedef struct ospfv2_hdr ospf_hdr;
typedef struct ospfv2_lsu_hdr lsu_hdr;
typedef struct ospfv2_hello_hdr hello_hdr;

static int seqnum = 0;
neighbor *head = NULL;
pthread_mutex_t neighbor_t;
bool lock = false;


/*---------------------------------------------------------------------
 * Method: pwospf_init(..)
 *
 * Sets up the internal data structures for the pwospf subsystem
 *
 * You may assume that the interfaces have been created and initialized
 * by this point.
 *---------------------------------------------------------------------*/

int pwospf_init(struct sr_instance* sr)
{
	assert(sr);

	sr->ospf_subsys = (struct pwospf_subsys*)malloc(sizeof(struct
	                  pwospf_subsys));

	assert(sr->ospf_subsys);
	pthread_mutex_init(&neighbor_t, 0);
	pthread_mutex_init(&(sr->ospf_subsys->lock), 0);


	/* -- handle subsystem initialization here! -- */
	pthread_t *hello_thread, *lsa_thread, *monitor_thread;

	/* -- start thread subsystem -- */
	if ( pthread_create(&sr->ospf_subsys->thread, 0, pwospf_run_thread, sr)) {
		perror("pthread_create");
		assert(0);
	}

	pthread_create(&hello_thread, NULL, &sr_send_hello, sr);
	pthread_create(&monitor_thread, NULL, &sr_monitor_neighors, sr);
	pthread_create(&lsa_thread, NULL, &sr_send_lsu, sr);
	return 0; /* success */
} /* -- pwospf_init -- */

void remove_from_neighbors(neighbor *node)
{
	neighbor *temp = head;
	if (temp->ip == node->ip)
	{
		head = head->next;
		free(temp);
	}
	else
	{
		neighbor *prev = head;
		while (temp)
		{
			if (temp->ip == node->ip)
			{
				prev->next = temp->next;
				free(temp);
				return;
			}
			prev = temp;
			temp = temp->next;
		}
	}
}

void sr_monitor_neighors(void *arg)
{
	while (1)
	{
		//pthread_mutex_lock(&neighbor_t);
		if (lock)
		{
			neighbor *temp = head;
			time_t currTime = time(NULL);
			while (temp)
			{
				time_t pkt_time = temp->rcv_time;
				double diff = difftime(currTime, pkt_time);
				if (diff > OSPF_NEIGHBOR_TIMEOUT && temp->rid!=0)
				{
					temp->rid = 0;

					//	remove_from_neighbors(temp);
					printf("Time difference is %lf\n", diff);
					printf("Neighbor is down\n");
				}
				temp = temp->next;
			}
			lock = false;
			//pthread_mutex_unlock(&neighbor_t);
		}
		//	sleep(10);
	}
}
/*---------------------------------------------------------------------
 * Method: pwospf_lock
 *
 * Lock mutex associated with pwospf_subsys
 *
 *---------------------------------------------------------------------*/

void pwospf_lock(struct pwospf_subsys* subsys)
{
	if ( pthread_mutex_lock(&subsys->lock) )
	{ assert(0); }
} /* -- pwospf_subsys -- */

/*---------------------------------------------------------------------
 * Method: pwospf_unlock
 *
 * Unlock mutex associated with pwospf subsystem
 *
 *---------------------------------------------------------------------*/

void pwospf_unlock(struct pwospf_subsys* subsys)
{
	if ( pthread_mutex_unlock(&subsys->lock) )
	{ assert(0); }
} /* -- pwospf_subsys -- */

/*---------------------------------------------------------------------
 * Method: pwospf_run_thread
 *
 * Main thread of pwospf subsystem.
 *
 *---------------------------------------------------------------------*/

static void* pwospf_run_thread(void* arg)
{
	struct sr_instance* sr = (struct sr_instance*)arg;


	while (1)
	{
		/* -- PWOSPF subsystem functionality should start  here! -- */

		pwospf_lock(sr->ospf_subsys);
		//printf(" pwospf subsystem sleeping \n");
		pwospf_unlock(sr->ospf_subsys);
		sleep(2);
		//  sr_send_hello(sr);

		//printf(" pwospf subsystem awake \n");
	}
} /* -- run_ospf_thread -- */

void sr_send_hello(void* arg)
{
	while (1)
	{
		//sleep(10);
		if (!lock)
		{
			lock = true;
			struct sr_instance* sr = (struct sr_instance *)arg;
			//printf("coming to hello\n");
			struct sr_ethernet_hdr* ethhdr = ((struct sr_ethernet_hdr*)(malloc(sizeof(struct sr_ethernet_hdr))));
			struct ip* iphdr = ((struct ip*)(malloc(sizeof(struct ip))));
			struct ospfv2_hdr* ospfhdr = ((struct ospfv2_hdr*)(malloc(sizeof(struct ospfv2_hdr))));
			struct ospfv2_hello_hdr* hellohdr = ((struct ospfv2_hello_hdr*)(malloc(sizeof(struct ospfv2_hello_hdr))));

			ethhdr->ether_type =  htons(ETHERTYPE_IP);
			int i;
			for (i = 0; i < ETHER_ADDR_LEN; i++)
			{
				ethhdr->ether_dhost[i] = htons(0xff);
			}

			iphdr->ip_dst.s_addr = htonl(OSPF_AllSPFRouters);
			iphdr->ip_hl = 5;
			iphdr->ip_tos = 0;
			iphdr->ip_off = IP_DF;
			iphdr->ip_id = rand();
			iphdr->ip_ttl = 64;
			iphdr->ip_p = 89;
			iphdr->ip_len = htons(sizeof(struct ip) + sizeof(struct ospfv2_hdr) + (sizeof(struct ospfv2_hello_hdr)));

			ospfhdr->aid = 171;
			ospfhdr->autype = 0;
			ospfhdr->audata = 0;
			ospfhdr->version = 2;
			ospfhdr->type = 1;
			ospfhdr->len = htons(sizeof(struct ospfv2_hdr) + (sizeof(struct ospfv2_hello_hdr)));

			hellohdr->nmask = htonl(0xfffffffe);
			hellohdr->helloint = OSPF_DEFAULT_HELLOINT;
			hellohdr->padding = 0;

			struct sr_if* iflist = sr->if_list;
			while (iflist)
			{
				for (i = 0; i < ETHER_ADDR_LEN; i++)
				{
					ethhdr->ether_shost[i] = iflist->addr[i];
				}
				struct in_addr temp1;
				temp1.s_addr = iflist->ip;
				iphdr->ip_src = temp1;
				iphdr->ip_sum = 0;
				iphdr->ip_sum = packet_checksum((uint16_t*)iphdr, sizeof(struct ip));

				ospfhdr->rid = iflist->ip;
				printf("coming to hello\n");
				int i;
				for (i = 0; i < ETHER_ADDR_LEN; i++)
				{
					ethhdr->ether_dhost[i] = htons(0xff);
				}

				iphdr->ip_dst.s_addr = htonl(OSPF_AllSPFRouters);



				ospfhdr->csum = 0;

				uint8_t * pkt = malloc(sizeof(struct sr_ethernet_hdr) + iphdr->ip_len);

				memcpy(pkt, ethhdr, sizeof(struct sr_ethernet_hdr));
				memcpy(pkt + sizeof(struct sr_ethernet_hdr), iphdr, sizeof(struct ip));
				memcpy(pkt + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip), ospfhdr,
				       sizeof(struct ospfv2_hdr));
				memcpy(pkt + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr),
				       hellohdr, sizeof(struct ospfv2_hello_hdr));

				((struct ospfv2_hdr*)(pkt + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip)))->csum =
				    packet_checksum(pkt + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip), sizeof(struct ospfv2_hdr) +
				                    sizeof(struct ospfv2_hello_hdr));
				char *intf = iflist->name;
				sr_send_packet(sr, pkt, (sizeof(struct sr_ethernet_hdr) + sizeof(ip_hdr) + sizeof(ospf_hdr) +
				                         sizeof(hello_hdr) ), intf);
				printf("finished\n\n\n\n");
				free(pkt);
				iflist = iflist->next;
			}

			sleep(10);
		}

	}

}

void handle_ospf_packet(struct sr_instance *sr, uint8_t *packet) {

	ethernet_hdr *ethHdr = (ethernet_hdr *)packet;
	ip_hdr *ipHdr = (ip_hdr*)(packet + sizeof(ethernet_hdr));
	ospf_hdr *ospfHdr = (ospf_hdr*)(packet + sizeof(ethernet_hdr) + sizeof(ip_hdr));
	printf("Entered handle_ospf_packet\n");
	if (ospfHdr->type == 1)
	{
		handle_hello_packet(sr, packet);
	}
	else if (ospfHdr->type == 4) {

		handle_lsu_packet(sr, packet);
	}
}

bool check_neighbor_exists(uint32_t ipAddr)
{
	neighbor *temp = head;
	while (temp) {
		if (temp->ip == ipAddr)
			return true;
		temp = temp->next;
	}
	return false;
}

void handle_lsu_packet(struct sr_instance *sr, uint8_t *packet)
{
	assert(sr);
	assert(packet);
	ethernet_hdr *ethHdr = (ethernet_hdr *)packet;
	ip_hdr *ipHdr = (ip_hdr *)(packet + sizeof(ethernet_hdr));
	ospf_hdr *ospfHdr = (ospf_hdr *)(packet + sizeof(ethernet_hdr) + sizeof(ip_hdr));
	lsu_hdr *lsuHdr = (lsu_hdr *)(packet + sizeof(ethernet_hdr) + sizeof(ip_hdr) + sizeof(ospf_hdr));
	lsu_pkt *ads = (lsu_pkt *)(packet + sizeof(ethernet_hdr) + sizeof(ip_hdr) + sizeof(ospf_hdr) + sizeof(lsu_hdr));
	assert(ads);
	int i, j;
	struct sr_if *iflist = sr->if_list;

	struct in_addr temp;
	temp.s_addr = ipHdr->ip_src.s_addr;
	printf("Sender address of IP packet %s\n", inet_ntoa(ipHdr->ip_src));
	printf("Destination address of IP packet %s\n", inet_ntoa(ipHdr->ip_dst));

	for (i = 0; i < 3; i++)
	{

		temp.s_addr = ads[i].subnet;
		printf("Subnet : %s\n", inet_ntoa(temp));
		temp.s_addr = ads[i].mask;
		printf("Mask : %s\n", inet_ntoa(temp));
		temp.s_addr = ads[i].rid;
		printf("RID : %s\n",inet_ntoa(temp));
		
		temp.s_addr = ipHdr->ip_src.s_addr;
		printf("NID : %s\n",inet_ntoa(temp));
		printf("***************\n");
		
		if(check_topology_entry(ads[i].subnet,ipHdr->ip_src.s_addr))
		{
			uint16_t seqNum = get_sequence_number(ads[i].subnet,ipHdr->ip_src.s_addr);
			if(seqNum<=seqnum)
			{
				printf("Sequence number is same as existing database. No update required\n");
			}
			else
			{
				printf("Sequence number is greater. Will update\n");
			}
		}
		else
		{
			printf("Adding entry to topology database\n");
			add_topology_entry(ads[i].rid,ads[i].subnet,ads[i].mask,ipHdr->ip_src.s_addr,lsuHdr->seq);
		}

	}
	print_topology();
	update_routing_table(sr);
	print_routing_table(sr);
}

void print_neighbors()
{
	neighbor *temp = head;
	printf("Neighbors are:\n");
	struct in_addr temp_ip;
	while (temp)
	{
		temp_ip.s_addr = temp->ip;
		printf("IP Address - %s\n", inet_ntoa(temp_ip));
		temp_ip.s_addr = temp->mask;
		printf("Mask - %s\n", inet_ntoa(temp_ip));
		printf("Received time - %s\n", ctime(&(temp->rcv_time)));
		printf("Interface is %s\n", temp->intf);
		printf("---------\n");
		temp = temp->next;
		
	}
}

void handle_hello_packet(struct sr_instance *sr, uint8_t *packet)
{
	ethernet_hdr *ethHdr = (ethernet_hdr *)packet;
	ip_hdr *ipHdr = (ip_hdr*)(packet + sizeof(ethernet_hdr));
	ospf_hdr *ospfHdr = (ospf_hdr*)(packet + sizeof(ethernet_hdr) + sizeof(ip_hdr));
	hello_hdr *helloHdr = (hello_hdr*)(packet + sizeof(ethernet_hdr) + sizeof(ip_hdr) + sizeof(ospf_hdr));
	printf("Entered handle_hello_packet\n");
	neighbor *temp = head;
	time_t curr_time = time(NULL);
	if (head == NULL)
	{
		head = (neighbor*)malloc(sizeof(neighbor));
		head->ip = ospfHdr->rid;
		head->mask = helloHdr->nmask;
		head->rcv_time = curr_time;
		head->intf = get_interface(sr, ipHdr->ip_src.s_addr);
		head->rid = ipHdr->ip_src.s_addr;
		head->next = NULL;
	}
	else
	{
		if (check_neighbor_exists(ospfHdr->rid) == false)
		{
			while (temp->next)
				temp = temp->next;
			temp->next = (neighbor *)malloc(sizeof(neighbor));
			temp->next->ip = ospfHdr->rid;
			temp->next->mask = helloHdr->nmask;
			temp->next->rcv_time = curr_time;
			temp->next->intf = get_interface(sr, ipHdr->ip_src.s_addr);
			temp->next->rid = ipHdr->ip_src.s_addr;
			temp->next->next = NULL;
		}
		else
		{
			while (temp)
			{
				if (temp->ip == ipHdr->ip_src.s_addr)
				{
					temp->rcv_time = curr_time;
					temp->rid = temp->ip;
					break;
				}
				temp = temp->next;
			}
			printf("Neighbor already exists\n");
		}
	}
	
	temp = head;
	int count=0;
	while(temp)
	{
		count++;
		temp=temp->next;
	}
	if(count>=2)
	{
		struct sr_if *iflist = sr->if_list;
		while(iflist)
		{
			bool result = check_interface_in_neighbors(iflist->name);
			if(!result)
				break;
			iflist=iflist->next;
		}
		if(!iflist)
			return;
		temp=head;
		while(temp->next)
			temp=temp->next;
		temp->next = (malloc)(sizeof(neighbor));
		temp->next->rid=0;
		temp->next->ip = iflist->ip;
		temp->next->intf = iflist->name;
		temp->next->next = NULL;
		temp->next->rcv_time = curr_time;

	}
	print_neighbors();
}

bool check_interface_in_neighbors(char *intf)
{
	neighbor *temp = head;
	while (temp)
	{
		if (strcmp(temp->intf, intf) == 0)
			return true;
		temp = temp->next;
	}
	return false;
}
char* get_interface(struct sr_instance *sr, uint32_t ip)
{
	struct sr_if *iflist = sr->if_list;
	int max = 0;
	char *intf;
	while (iflist)
	{
		if ((iflist->ip & ip) > max)
		{
			max = iflist->ip & ip;
			intf = iflist->name;
		}
		iflist = iflist->next;
	}

	return intf;
}

void sr_send_lsu(void* arg)
{
	while (1)
	{
		if (head)
		{
			sleep(20);

			int j;

			struct sr_instance *sr = (struct sr_instance*)arg;
			struct sr_ethernet_hdr* ethhdr = ((struct sr_ethernet_hdr*)(malloc(sizeof(struct sr_ethernet_hdr))));
			struct ip* iphdr = ((struct ip*)(malloc(sizeof(struct ip))));
			struct ospfv2_hdr* ospfhdr = ((struct ospfv2_hdr*)(malloc(sizeof(struct ospfv2_hdr))));
			struct ospfv2_lsu_hdr* lsuhdr = ((struct ospfv2_lsu_hdr*)(malloc(sizeof(struct ospfv2_lsu_hdr))));
			struct ospfv2_lsu* lsa = ((struct ospfv2_lsu*)(malloc(sizeof(struct ospfv2_lsu))));

			ethhdr->ether_type =  htons(ETHERTYPE_IP);
			int i;
			for (i = 0; i < ETHER_ADDR_LEN; i++)
			{
				ethhdr->ether_dhost[i] = htons(0xff);
			}

			iphdr->ip_hl = 5;
			iphdr->ip_tos = 0;

			iphdr->ip_off = IP_DF;
			iphdr->ip_id = rand();
			iphdr->ip_ttl = 64;
			iphdr->ip_p = 89;
			iphdr->ip_len = htons(sizeof(struct ip) + sizeof(struct ospfv2_hdr) + sizeof(struct ospfv2_lsu_hdr) +
			                      (sizeof(struct ospfv2_lsu) * 3));
			ospfhdr->aid = 171;
			ospfhdr->csum = 0;
			ospfhdr->autype = 0;
			ospfhdr->audata = 0;
			ospfhdr->version = 2;
			ospfhdr->type = 4;
			ospfhdr->len = htons(sizeof(struct ospfv2_hdr) + sizeof(struct ospfv2_lsu_hdr) + (sizeof(struct ospfv2_lsu) * 3));
			struct sr_if* iflist = sr->if_list;
			neighbor *temp = head;
			lsuhdr->unused = 0;
			lsuhdr->ttl = 64;
			lsuhdr->num_adv = 3;
			seqnum++;
			lsuhdr->seq = seqnum;
			iphdr->ip_sum = 0;
			lsu_pkt *ads = (lsu_pkt *)malloc(3 * sizeof(lsu_pkt));
			uint8_t *pkt = (uint8_t *)malloc(sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + 
					sizeof(struct ospfv2_hdr) + sizeof(struct ospfv2_lsu_hdr) +
				                                 (sizeof(struct ospfv2_lsu) * 3));
			while (temp)
			{
				lsu_pkt ad;
				if(temp->rid!=0)
				{
					uint32_t router_ip = get_router_ip(sr, temp->intf);
					uint32_t subnet = temp->ip & router_ip;
					uint32_t default_mask = 0xffffffff;
					uint32_t mask = ((temp->ip)^router_ip)^default_mask;
					uint32_t rid = temp->ip;
					
					ad.subnet = subnet;
					ad.mask = mask;
					ad.rid = temp->rid;
					ads[j++] = ad;
				}
				else
				{
					uint32_t router_ip = get_router_ip(sr, temp->intf);
					uint32_t subnet = temp->ip & router_ip;
					uint32_t mask = htonl(0xfffffffe);
				//	uint32_t mask = ((temp->ip)^router_ip)^default_mask;
					uint32_t rid = 0;
					
					ad.subnet = subnet;
					ad.mask = mask;
					ad.rid = temp->rid;
					ads[j++] = ad;
				}
				temp = temp->next;
			}
			temp = head;
			while (temp)
			{
				iphdr->ip_dst.s_addr = temp->ip;
				iphdr->ip_src.s_addr = get_router_ip(sr, temp->intf);



				char *mac = get_mac_address(sr, temp->intf);
				for (i = 0; i < 6; i++)
					ethhdr->ether_shost[i] = mac[i];

				iphdr->ip_sum = packet_checksum((uint16_t*)iphdr, sizeof(struct ip));
				ospfhdr->csum = packet_checksum((uint16_t *)ospfhdr, sizeof(struct ospfv2_hdr));

				// lsuhdr->ads = ads;
				

				memcpy(pkt, ethhdr, sizeof(struct sr_ethernet_hdr));
				memcpy(pkt + sizeof(struct sr_ethernet_hdr), iphdr, sizeof(struct ip));
				memcpy(pkt + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip), ospfhdr,
				       sizeof(struct ospfv2_hdr));
				memcpy(pkt + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr),
				       lsuhdr, sizeof(struct ospfv2_lsu_hdr));
				memcpy(pkt + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr) +
				 sizeof(struct ospfv2_lsu_hdr),
				       ads, 3 * sizeof(lsu_pkt));


				sr_send_packet(sr, pkt, sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr) +
				 sizeof(struct ospfv2_lsu_hdr) + 3 * sizeof(lsu_pkt), temp->intf);
				printf("Finished sending LSA\n");
				

				temp = temp->next;

			}
			free(ads);
			free(pkt);
		}
		sleep(20);
	}


}

uint32_t get_router_ip(struct sr_instance *sr, char *intf)
{
	struct sr_if *iflist = sr->if_list;
	while (iflist)
	{
		if (strcmp(iflist->name, intf) == 0)
		{
			return iflist->ip;
		}
		iflist = iflist->next;
	}
	return 1;
}

char* get_mac_address(struct sr_instance *sr, char *intf)
{
	struct sr_if *iflist = sr->if_list;
	while (iflist)
	{
		if (strcmp(iflist->name, intf) == 0)
		{
			return iflist->addr;
		}
		iflist = iflist->next;
	}
	return NULL;
}
/*void sr_add_to_advertisements(lsu_pkt *lsu_head,uint32_t subnet, uint32_t mask)
{
	if(lsu_head==NULL)
	{
		lsu_head = (lsu_pkt *)malloc(sizeof(lsu_pkt));
		lsu_head->subnet = subnet;
		lsu_head->mask = mask;
		lsu_head->next = NULL;
	}
	else
	{
		lsu_pkt *temp = lsu_head;
		while(temp->next)
			temp = temp->next;
		temp->next = (lsu_pkt *)malloc(sizeof(lsu_pkt));
		temp->next->subnet = subnet;
		temp->next->mask = mask;
		temp->next->next = NULL;
	}
}*/













