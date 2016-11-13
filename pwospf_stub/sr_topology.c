#include "sr_topology.h"
#include<limits.h>

static top_entry *top_head=NULL;
void add_topology_entry(uint32_t router_id,uint32_t net_prefix,uint32_t	net_mask,uint32_t next_hop,uint16_t seqNum)
{
	if(top_head==NULL)
	{
		printf("Empty list. Adding\n");
		top_head= malloc(sizeof(top_entry));
		top_head->router_id.s_addr=router_id;
		top_head->net_prefix.s_addr = net_prefix;
		
		top_head->net_mask.s_addr = net_mask;
		top_head->next_hop.s_addr = next_hop;
		top_head->sequence_num = seqNum;
		
		top_head->next = NULL;
	}
	else
	{
		top_entry *temp = top_head;
		while(temp->next)
			temp=temp->next;
		temp->next = (malloc)(sizeof(top_entry));
		temp->next->router_id.s_addr=router_id;
		temp->next->net_prefix.s_addr = net_prefix;
		
		temp->next->net_mask.s_addr = net_mask;
		temp->next->next_hop.s_addr = next_hop;
		temp->next->sequence_num = seqNum;
		
		temp->next->next = NULL;
	}
	
}
void remove_topology_entry(uint32_t subnet,uint32_t next_hop)
{
	top_entry *temp=top_head,*prev=NULL;
	while(temp)
	{
		
		if(temp->net_prefix.s_addr == subnet && temp->next_hop.s_addr == next_hop)
		{
			prev->next = temp->next;
			free(temp);
			return;
		}
			
		prev=temp;	
		temp=temp->next;
	}
	
}

bool check_topology_entry(uint32_t subnet,uint32_t next_hop)
{
	top_entry *temp=top_head;
	while(temp)
	{
		if(temp->net_prefix.s_addr == subnet && temp->next_hop.s_addr == next_hop)
			return true;
		temp=temp->next;
	}
	return false;
}

uint16_t get_sequence_number(uint32_t subnet,uint32_t next_hop)
{
	top_entry *temp=top_head;
	while(temp)
	{
		if(temp->net_prefix.s_addr == subnet && temp->next_hop.s_addr == next_hop)
			return temp->sequence_num;
		temp=temp->next;
	}
	return INT_MAX;	
}

void print_topology()
{
	top_entry *temp=top_head;
	struct in_addr temp1;
	printf("TOPOLOGY\n");
	printf("DST\t\tNXT\t\tMSK\t\tPRE\t\tSeq\n");
	while(temp)
	{
		//printf("Test\n");
		temp1=temp->router_id;
		printf("%s\t",inet_ntoa(temp1));
		printf("\t");
		temp1=temp->next_hop;
		printf("%s\t",inet_ntoa(temp1));
		temp1=temp->net_mask;
		printf("%s\t",inet_ntoa(temp1));
		temp1=temp->net_prefix;
		printf("%s\t",inet_ntoa(temp1));
		printf("%d\n",temp->sequence_num);
		temp=temp->next;
	}


}

