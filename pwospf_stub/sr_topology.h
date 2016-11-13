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

struct ospfv2_topo_entry
{
    struct in_addr router_id;     /* -- router id -- */
    struct in_addr net_prefix;       /* -- network prefix -- */
    struct in_addr net_mask;      /* -- network mask -- */
    struct in_addr next_hop;      /* -- next hop -- */
    uint16_t sequence_num;        /* -- sequence number of the LSU -- */
    int age;                      /* -- LSA age -- */
    struct ospfv2_topo_entry *next;
}__attribute__((packed));

typedef struct ospfv2_topo_entry top_entry;

void add_topology_entry(uint32_t,uint32_t,uint32_t,uint32_t,uint16_t);
void remove_topology_entry(uint32_t,uint32_t);
bool check_topology_entry(uint32_t,uint32_t);
uint16_t get_sequence_number(uint32_t,uint32_t);
void print_topology();

