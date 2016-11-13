/*-----------------------------------------------------------------------------
 * file:  sr_pwospf.h
 * date:  Tue Nov 23 23:21:22 PST 2004 
 * Author: Martin Casado
 *
 * Description:
 *
 *---------------------------------------------------------------------------*/

#ifndef SR_PWOSPF_H
#define SR_PWOSPF_H

#include <pthread.h>
#include <stdint.h>
#include <time.h>

/* forward declare */
struct sr_instance;

struct pwospf_subsys
{
    /* -- pwospf subsystem state variables here -- */


    /* -- thread and single lock for pwospf subsystem -- */
    pthread_t thread;
    pthread_mutex_t lock;
};

int pwospf_init(struct sr_instance* sr);
void handle_ospf_packet(struct sr_instance *sr, uint8_t *packet);

struct ospf_neighbor
{
	uint32_t ip;
	uint32_t rid;
	uint32_t mask;
	time_t rcv_time;
	char *intf;
	struct ospf_neighbor *next;
};

typedef struct ospf_neighbor neighbor;


#endif /* SR_PWOSPF_H */
