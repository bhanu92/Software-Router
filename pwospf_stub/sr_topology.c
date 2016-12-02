#include "sr_topology.h"
#include <limits.h>
#include <stdbool.h>

top_entry *top_head = NULL;
void add_topology_entry(uint32_t router_id, uint32_t subnet, uint32_t net_mask, uint32_t neighbor_id, uint32_t next_hop, uint16_t seqNum)
{

	if (top_head == NULL)
	{
		printf("Empty list. Adding\n");
		top_head = malloc(sizeof(top_entry));
		top_head->router_id.s_addr = router_id;
		top_head->subnet.s_addr = subnet;
		top_head->net_mask.s_addr = net_mask;
		top_head->neighbor_id.s_addr = neighbor_id;
		top_head->next_hop.s_addr = next_hop;
		top_head->sequence_num = seqNum;

		top_head->next = NULL;
	}
	else
	{
		top_entry *temp = top_head;
		while (temp->next)
			temp = temp->next;
		temp->next = (malloc)(sizeof(top_entry));
		temp->next->router_id.s_addr = router_id;
		temp->next->subnet.s_addr = subnet;
		temp->next->net_mask.s_addr = net_mask;
		temp->next->neighbor_id.s_addr = neighbor_id;
		temp->next->next_hop.s_addr = next_hop;
		temp->next->sequence_num = seqNum;

		temp->next->next = NULL;
	}

}
void remove_topology_entry(uint32_t subnet, uint32_t next_hop)
{
	top_entry *temp = top_head, *prev = NULL;
	while (temp)
	{

		if (temp->subnet.s_addr == subnet && temp->next_hop.s_addr == next_hop)
		{
			prev->next = temp->next;
			free(temp);
			return;
		}

		prev = temp;
		temp = temp->next;
	}

}

bool check_topology_entry(uint32_t rid, uint32_t subnet)
{
	top_entry *temp = top_head;
	while (temp)
	{
		if (temp->router_id.s_addr == rid && temp->subnet.s_addr == subnet)
			return true;
		temp = temp->next;
	}
	return false;
}

uint16_t get_sequence_number(uint32_t rid, uint32_t next_hop)
{
	top_entry *temp = top_head;
	while (temp)
	{
		if (temp->router_id.s_addr == rid && temp->next_hop.s_addr == next_hop)
			return temp->sequence_num;
		temp = temp->next;
	}
	return INT_MAX;
}

void print_topology()
{
	top_entry *temp = top_head;
	struct in_addr temp1;
	printf("TOPOLOGY\n");
	printf("RID\t\tSUB\t\tMSK\t\tNID\t\tNHP\t\tSeq\n");
	while (temp)
	{
		//printf("Test\n");
		printf("%s\t", inet_ntoa(temp->router_id));
		printf("%s\t", inet_ntoa(temp->subnet));
		printf("%s\t", inet_ntoa(temp->net_mask));
		printf("%s\t", inet_ntoa(temp->neighbor_id));
		printf("%s\t", inet_ntoa(temp->next_hop));
		printf("%d\n", temp->sequence_num);
		temp = temp->next;
	}


}

void update_routing_table(struct sr_instance *sr)
{
	printf("Updating Routing table\n");
	top_entry *temp = top_head;

	while (temp)
	{
		char *intf = fetch_interface_for_destination(sr, temp->next_hop.s_addr);
		if (intf && !check_routing_table(sr, temp->subnet.s_addr, temp->next_hop.s_addr))
		{
			add_entry_to_routing_table(sr, temp->subnet.s_addr, temp->next_hop.s_addr, temp->net_mask.s_addr, intf);
		}
		else
		{
			printf("Next hop is 0\n");
		}
		temp = temp->next;
	}
	struct sr_if *iflist = sr->if_list;
	while (iflist)
	{
		bool existsInRoutingTable = check_interface_in_routing_table(sr, iflist->name);
		if (!existsInRoutingTable)
			break;
		iflist = iflist->next;
	}
	if (iflist)
		add_entry_to_routing_table(sr, (iflist->ip & iflist->mask), 0, iflist->mask, iflist->name);

	adding_defualt_route(sr);
}

void adding_defualt_route(struct sr_instance *sr) {

	struct sr_rt* rtable = sr->routing_table;
	bool default_route = false;

	while (rtable) {
		if (rtable->dest.s_addr == 0) {
			default_route = true;
			break;
		}
		rtable = rtable->next;
	}
	struct sr_if* iflist = sr->if_list;
	if (default_route == false) {
		while (iflist) {
			if (strcmp(iflist, "eth0")==0) {
				break;
			}
			iflist = iflist->next;
		}
		add_entry_to_routing_table(sr, 0, iflist->neighbor_ip, 0, iflist->name);
	}

}

char* fetch_interface_for_destination(struct sr_instance *sr, uint32_t dest)
{
	if (dest == 0)
		return NULL;
	struct sr_if *temp = sr->if_list;
	struct in_addr temp1;
	temp1.s_addr = dest;
	printf("Fetching interface for %s\n", inet_ntoa(temp1));
	uint32_t max = 0;
	char *longestMatch;
	while (temp)
	{
		uint32_t val = temp->ip & dest;
		if (val > max) {
			max = val;
			longestMatch = temp->name;
		}
		temp = temp->next;
	}

	return longestMatch;

	return longestMatch;
}

void add_entry_to_routing_table(struct sr_instance *sr, uint32_t dest_prefix, uint32_t next_hop, uint32_t mask, char *intf)
{
	struct sr_rt *routing_table = sr->routing_table;
	int i;
	if (sr->routing_table == NULL)
	{
		printf("Routing table is empty.Adding\n");
		sr->routing_table = malloc(sizeof(struct sr_rt));
		sr->routing_table->dest.s_addr = dest_prefix;
		sr->routing_table->gw.s_addr = next_hop;
		sr->routing_table->mask.s_addr = mask;
		for (i = 0; i < 4; i++)
			sr->routing_table->interface[i] = intf[i];
		sr->routing_table->next = NULL;
	}
	else
	{
		while (routing_table->next)
			routing_table = routing_table->next;
		routing_table->next = malloc(sizeof(struct sr_rt));
		routing_table->next->dest.s_addr = dest_prefix;
		routing_table->next->gw.s_addr = next_hop;
		routing_table->next->mask.s_addr = mask;
		for (i = 0; i < 4; i++)
			routing_table->next->interface[i] = intf[i];
		routing_table->next->next = NULL;
	}

}

bool check_routing_table(struct sr_instance *sr, uint32_t dest_prefix, uint32_t next_hop)
{
	struct sr_rt *routing_table = sr->routing_table;
	while (routing_table)
	{
		if (routing_table->dest.s_addr == dest_prefix && routing_table->gw.s_addr == next_hop) {
			printf("Already exists in routing table\n");
			return true;
		}
		routing_table = routing_table->next;
	}
	return false;
}

bool check_router_interfaces(struct sr_instance *sr, uint32_t ip)
{
	struct sr_if *iflist = sr->if_list;

	while (iflist)
	{
		if (iflist->ip == ip && ip != 0) {
			printf("Destination is router itself. So excluding\n");
			return true;
		}
		iflist = iflist->next;
	}

	return false;

}

bool check_interface_in_routing_table(struct sr_instance *sr, char *intf)
{
	struct sr_rt *routing_table = sr->routing_table;
	while (routing_table)
	{
		if (strcmp(routing_table->interface, intf) == 0)
			return true;
		routing_table = routing_table->next;
	}
	return false;
}

void print_routing_table(struct sr_instance *sr)
{

	struct sr_rt *routing_table = sr->routing_table;
	int i;
	printf("DST\t\tGW\t\tMASK\t\tINTF\n");
	while (routing_table)
	{
		struct in_addr temp;
		temp = routing_table->dest;
		printf("%s\t", inet_ntoa(temp));
		temp = routing_table->gw;
		printf("%s\t", inet_ntoa(temp));
		temp = routing_table->mask;
		printf("%s\t", inet_ntoa(temp));
		printf("%s\t\n", routing_table->interface);
		routing_table = routing_table->next;
	}
}