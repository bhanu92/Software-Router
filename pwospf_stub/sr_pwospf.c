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

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ether.h>

/* -- declaration of main thread function for pwospf subsystem --- */
static void* pwospf_run_thread(void* arg);
void sr_send_lsu(void* arg);
void sr_send_hello(void* arg);
void sr_monitor_neighors(void *arg);
typedef struct ospfv2_lsu lsu_pkt;
//void sr_add_to_advertisements(lsu_pkt *head,uint32_t subnet, uint32_t mask);
typedef struct sr_ethernet_hdr ethernet_hdr;
typedef struct ip ip_hdr;
typedef struct ospfv2_hdr ospf_hdr;
typedef struct ospfv2_lsu_hdr lsu_hdr;
typedef struct ospfv2_hello_hdr hello_hdr;

static int seqnum = 0;
neighbor *head=NULL;
pthread_mutex_t neighbor_t;
bool lock=false;

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
    pthread_mutex_init(&neighbor_t,0);
    pthread_mutex_init(&(sr->ospf_subsys->lock), 0);


    /* -- handle subsystem initialization here! -- */
    pthread_t *hello_thread,*lsa_thread,*monitor_thread;

    /* -- start thread subsystem -- */
    if( pthread_create(&sr->ospf_subsys->thread, 0, pwospf_run_thread, sr)) {
        perror("pthread_create");
        assert(0);
    }

    pthread_create(&hello_thread,NULL,&sr_send_hello,sr);
    pthread_create(&monitor_thread,NULL,&sr_monitor_neighors,sr);
    pthread_create(&lsa_thread,NULL,&sr_send_lsu,sr);
    return 0; /* success */
} /* -- pwospf_init -- */

void remove_from_neighbors(neighbor *node)
{
	neighbor *temp=head;
	if(temp->ip==node->ip)
	{
		head=head->next;
		free(temp);
	}
	else
	{
		neighbor *prev=head;
		while(temp)
		{	
			if(temp->ip==node->ip)
			{
				prev->next = temp->next;
				free(temp);
				return;
			}
			prev=temp;
			temp=temp->next;
		}
	}
}

void sr_monitor_neighors(void *arg)
{
	while(1)
	{		
		//pthread_mutex_lock(&neighbor_t);
		if(lock)
		{
			neighbor *temp = head;
			time_t currTime = time(NULL);
			while(temp)
			{			
				time_t pkt_time = temp->rcv_time;
				double diff = difftime(currTime,pkt_time);
				if(diff>OSPF_NEIGHBOR_TIMEOUT)
				{
					temp->rid=0;

				//	remove_from_neighbors(temp);
					printf("Time difference is %lf\n",diff);
					printf("Neighbor is down\n");				
				}
				temp=temp->next;
			}
			lock=false;
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


    while(1)
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
	while(1)
	{
		if(!lock)
		{
			lock=true;
			struct sr_instance* sr = (struct sr_instance *)arg;
			printf("coming to hello\n");
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
			iphdr->ip_len = htons(sizeof(struct ip) + sizeof(struct ospfv2_hdr) +(sizeof(struct ospfv2_hello_hdr)));

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
			while(iflist)
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

				ospfhdr->rid = iflist->ip;struct sr_instance* sr = (struct sr_instance *)arg;
				printf("coming to hello\n");
				int i;
				for (i = 0; i < ETHER_ADDR_LEN; i++)
				{
					ethhdr->ether_dhost[i] = htons(0xff);
				}

				iphdr->ip_dst.s_addr = htonl(OSPF_AllSPFRouters);
			
			
				
				ospfhdr->csum = 0;

				uint8_t * pkt = malloc(sizeof(struct sr_ethernet_hdr)+ iphdr->ip_len);

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
				sr_send_packet(sr, pkt, (sizeof(struct sr_ethernet_hdr)+ sizeof(ip_hdr) + sizeof(ospf_hdr) + 
					sizeof(hello_hdr) ), intf);
				printf("finished\n\n\n\n");
				free(pkt);
				iflist = iflist->next;
			}
			
			sleep(5);
		}

	}

}

void handle_ospf_packet(struct sr_instance *sr, uint8_t *packet){

	ethernet_hdr *ethHdr = (ethernet_hdr *)packet;
	ip_hdr *ipHdr = (ip_hdr*)(packet +sizeof(ethernet_hdr));
	ospf_hdr *ospfHdr = (ospf_hdr*)(packet + sizeof(ethernet_hdr)+sizeof(ip_hdr));
	printf("Entered handle_ospf_packet\n");
	if(ospfHdr->type == 1)
	{
			handle_hello_packet(sr,packet);
	}
	else if(ospfHdr->type==4){
		printf("Received OSPF advertisement\n");
		handle_lsu_packet(sr,packet);
	}
}

bool check_neighbor_exists(uint32_t ipAddr)
{
	neighbor *temp=head;
	while(temp){
		if(temp->ip==ipAddr)
			return true;
		temp=temp->next;
	}
	return false;
}

void handle_lsu_packet(struct sr_instance *sr,uint8_t *packet)
{
	assert(sr);
	assert(packet);
	ethernet_hdr *ethHdr = (ethernet_hdr *)packet;
	ip_hdr *ipHdr = (ip_hdr *)(packet + sizeof(ethernet_hdr));
	ospf_hdr *ospfHdr = (ospf_hdr *)(packet + sizeof(ethernet_hdr) + sizeof(ip_hdr));
	lsu_hdr *lsuHdr = (lsu_hdr *)(packet + sizeof(ethernet_hdr) + sizeof(ip_hdr) + sizeof(ospf_hdr));
	lsu_pkt *ads = (lsu_pkt *)(packet + sizeof(ethernet_hdr) + sizeof(ip_hdr) + sizeof(ospf_hdr) + sizeof(lsu_hdr));
	assert(ads);
	int i;
	struct in_addr temp;
	temp.s_addr = ipHdr->ip_src.s_addr;
		printf("Advertisement from neighbor %s\n",inet_ntoa(temp));
	for(i=0;i<3;i++)
	{
		
		temp.s_addr = ads[i].subnet;
		printf("Subnet - %s\t",inet_ntoa(temp));
		temp.s_addr = ads[i].mask;
		printf("IP Mask - %s\t\n",inet_ntoa(temp));

	}	
}

void print_neighbors()
{
	neighbor *temp=head;
	printf("Neighbors are:\n");
	struct in_addr temp_ip;
	while(temp)
	{
		temp_ip.s_addr =temp->ip;
		printf("IP Address - %s\n",inet_ntoa(temp_ip));
		temp_ip.s_addr = temp->mask;
		printf("Mask - %s\n",inet_ntoa(temp_ip));
		printf("Received time - %s\n",ctime(&(temp->rcv_time)));
		printf("---------\n");
		temp=temp->next;
	}
}

void handle_hello_packet(struct sr_instance *sr,uint8_t *packet)
{
	ethernet_hdr *ethHdr = (ethernet_hdr *)packet;
	ip_hdr *ipHdr = (ip_hdr*)(packet +sizeof(ethernet_hdr));
	ospf_hdr *ospfHdr = (ospf_hdr*)(packet + sizeof(ethernet_hdr)+sizeof(ip_hdr));
	hello_hdr *helloHdr = (hello_hdr*)(packet + sizeof(ethernet_hdr)+sizeof(ip_hdr)+ sizeof(ospf_hdr));
	printf("Entered handle_hello_packet\n");
	neighbor *temp=head;
	time_t curr_time = time(NULL);
	if(head==NULL)
	{
		head = (neighbor*)malloc(sizeof(neighbor));
		head->ip = ospfHdr->rid;
		head->mask = helloHdr->nmask;
		head->rcv_time = curr_time;
		head->next = NULL;
	}
	else
	{
		if(check_neighbor_exists(ospfHdr->rid)==false)
		{
			while(temp->next)
				temp=temp->next;
			temp->next = (neighbor *)malloc(sizeof(neighbor));
			temp->next->ip = ospfHdr->rid;
			temp->next->mask = helloHdr->nmask;
			temp->next->rcv_time = curr_time;
			temp->next->next = NULL;
		}
		else
		{
			while(temp)
			{
				if(temp->ip==ipHdr->ip_src.s_addr)
				{
					temp->rcv_time = curr_time;
					temp->rid = temp->ip;
					break;
				}
				temp=temp->next;
			}
			printf("Neighbor already exists\n");
		}
	}
	print_neighbors();
}




void sr_send_lsu(void* arg)
{
	while(1)
	{
		if(head)
		{
			sleep(20);
			lsu_pkt *ads=(lsu_pkt *)malloc(3*sizeof(lsu_pkt));
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
		    
		    while(iflist)
			{
				j=0;
			    while(temp)
			    {	
				if(temp->rid!=0 && j<3)
			    	{
			//	printf("Address and interface -%s \t%s\n",ether_ntoa(iflist->addr),iflist->name);
					iphdr->ip_src.s_addr = iflist->ip;	
		

				    	ospfhdr->rid = iflist->ip;
				    	iphdr->ip_dst.s_addr = temp->ip;
		
					uint32_t subnet = (temp->ip)&(iflist->ip);
					uint32_t default_mask = 0xffffffff;
				 	uint32_t mask = ((temp->ip)^(iflist->ip))^default_mask;
					struct in_addr temp1;
					temp1.s_addr = subnet;
					printf("Subnet - %s\t",inet_ntoa(temp1));
					temp1.s_addr = mask;
					printf("Mask - %s\n",inet_ntoa(temp1));
					temp1.s_addr = iflist->ip;
					printf("IP Address of router %s\n",inet_ntoa(temp1));
					temp1.s_addr = temp->ip;
					printf("IP address of neighbor %s\n",inet_ntoa(temp1));
				 		
				 	lsu_pkt ad;
					ad.subnet = subnet;
					ad.mask = mask;
					ad.rid = temp->rid;
					ads[j++]=ad;
				}

				temp = temp->next;
			     }
				for (i = 0; i < ETHER_ADDR_LEN; i++)
				{
					ethhdr->ether_shost[i] = iflist->addr[i];
				}
			     printf("Address and interface -%s \t%s\n",ether_ntoa(iflist->addr),iflist->name);
			     iphdr->ip_sum = packet_checksum((uint16_t*)iphdr, sizeof(struct ip));
			     ospfhdr->csum = packet_checksum((uint16_t *)ospfhdr, sizeof(struct ospfv2_hdr));
			     
			    // lsuhdr->ads = ads;
		 	     uint8_t *pkt = (uint8_t *)malloc(sizeof(struct sr_ethernet_hdr)+sizeof(struct ip) + sizeof(struct ospfv2_hdr) + sizeof(struct ospfv2_lsu_hdr) +
					 (sizeof(struct ospfv2_lsu) * 3));

				memcpy(pkt, ethhdr, sizeof(struct sr_ethernet_hdr));
						memcpy(pkt + sizeof(struct sr_ethernet_hdr), iphdr, sizeof(struct ip));
						memcpy(pkt + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip), ospfhdr, 
							sizeof(struct ospfv2_hdr));
						memcpy(pkt + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr), 
							lsuhdr, sizeof(struct ospfv2_lsu_hdr));
memcpy(pkt + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr) + sizeof(struct ospfv2_lsu_hdr), 
							ads, 3*sizeof(lsu_pkt));
				char *intf = iflist->name;
				printf("Interface is %s\n",intf);

				sr_send_packet(sr,pkt,sizeof(struct sr_ethernet_hdr)+sizeof(struct ip)+sizeof(struct ospfv2_hdr)+sizeof(struct ospfv2_lsu_hdr)+3*sizeof(lsu_pkt),intf);
				printf("Finished sending LSA\n");
				free(pkt);
			     iflist = iflist->next;
			}
			sleep(10);
		}
	}
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













