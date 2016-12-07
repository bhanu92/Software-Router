#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>
#include<string.h>
#include<stdint.h>
#include "sr_pwospf.h"
#include "sr_router.h"
#include "sr_if.h"
#include "pwospf_protocol.h"
#include "sr_protocol.h"
#include "sr_rt.h"

struct ospfv2_topo_entry
{
    struct in_addr router_id;    
    struct in_addr subnet;       
    struct in_addr net_mask;
    struct in_addr neighbor_id;   
    struct in_addr next_hop;      
    uint16_t sequence_num;        
    int age;                      
    struct ospfv2_topo_entry *next;
}__attribute__((packed));

typedef struct ospfv2_topo_entry top_entry;

void add_topology_entry(uint32_t router_id, uint32_t subnet, uint32_t net_mask, uint32_t neighbor_id, uint32_t next_hop, uint16_t seqNum);
void remove_topology_entry(uint32_t,uint32_t);
bool check_topology_entry(uint32_t,uint32_t);
uint16_t get_sequence_number(uint32_t,uint32_t);
void print_topology();
void update_routing_table(struct sr_instance*);
void add_entry_to_routing_table(struct sr_instance *sr,uint32_t dest_prefix,uint32_t next_hop,uint32_t mask,char *intf);
bool check_interface_in_routing_table(struct sr_instance *sr,char *intf);
bool check_routing_table(struct sr_instance *sr,uint32_t dest_prefix,uint32_t next_hop);
char* fetch_interface_for_destination(struct sr_instance *sr,uint32_t dest);
void print_routing_table(struct sr_instance *sr);
bool check_router_interfaces(struct sr_instance *sr, uint32_t ip);
void adding_defualt_route(struct sr_instance *sr);
void clear_topology();
void clear_routing_entries(struct sr_instance *sr);
