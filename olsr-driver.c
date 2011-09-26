#include "ross.h"
#include "olsr.h"
#include <assert.h>

/**
 * @file
 * @brief OLSR Driver
 *
 * Simple driver to test out various functionalities in the OLSR impl.
 */

#define GRID_MAX 100
#define STAGGER_MAX 10
#define HELLO_DELTA 0.0001

static unsigned int nlp_per_pe = OLSR_MAX_NEIGHBORS;

// Used as scratch space for MPR calculations
unsigned g_Dy[OLSR_MAX_NEIGHBORS];
unsigned g_num_one_hop;
neigh_tuple g_mpr_one_hop[OLSR_MAX_NEIGHBORS];
unsigned g_num_two_hop;
two_hop_neigh_tuple g_mpr_two_hop[OLSR_MAX_2_HOP];
two_hop_neigh_tuple g_mpr_neigh_to_add;
char g_covered[BITNSLOTS(OLSR_MAX_2_HOP)];

/**
 * Initializer for OLSR
 */
void olsr_init(node_state *s, tw_lp *lp)
{
    hello *h;
    tw_event *e;
    olsr_msg_data *msg;
    tw_stime ts;
    
    //s->num_tuples = 0;
    s->num_neigh  = 0;
    s->num_two_hop = 0;
    s->num_mpr = 0;
    s->local_address = lp->gid;
    s->lng = tw_rand_unif(lp->rng) * GRID_MAX;
    s->lat = tw_rand_unif(lp->rng) * GRID_MAX;
    
    ts = tw_rand_unif(lp->rng) * STAGGER_MAX;
    
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->type = HELLO_TX;
    msg->originator = s->local_address;
    msg->lng = s->lng;
    msg->lat = s->lat;
    h = &msg->mt.h;
    h->num_neighbors = 0;
    //h->neighbor_addrs[0] = s->local_address;
    tw_event_send(e);
}

#define RANGE 40.0

static inline int out_of_radio_range(node_state *s, olsr_msg_data *m)
{
    const double range = RANGE;
    
    double sender_lng = m->lng;
    double sender_lat = m->lat;
    double receiver_lng = s->lng;
    double receiver_lat = s->lat;
    
    double dist = (sender_lng - receiver_lng) * (sender_lng - receiver_lng);
    dist += (sender_lat - receiver_lat) * (sender_lat - receiver_lat);
    
    dist = sqrt(dist);
    
    if (dist > range) {
        return 1;
    }
    
    return 0;
}

/**
 * Compute D(y) as described in the "MPR Computation" section.  Description:
 *
 * The degree of an one hop neighbor node y (where y is a member of N), is
 * defined as the number of symmetric neighbors of node y, EXCLUDING all the
 * members of N and EXCLUDING the node performing the computation.
 *
 * N is the subset of neighbors of the node, which are neighbors of the
 * interface I.
 */
static inline unsigned Dy(node_state *s, o_addr target)
{
    int i, j, in;
    o_addr temp[OLSR_MAX_NEIGHBORS];
    int temp_size = 0;
    
    for (i = 0; i < s->num_two_hop; i++) {
        
        in = 0;
        for (j = 0; j < s->num_neigh; j++) {
            if (s->twoHopSet[i].twoHopNeighborAddr == s->neighSet[j].neighborMainAddr) {
                // EXCLUDING all members of N...
                in = 1;
                continue;
            }
        }
        
        if (in) continue;
        
        if (s->twoHopSet[i].neighborMainAddr == target) {
            in = 0;
            // Add s->twoHopSet[i].twoHopNeighborAddr to this set
            for (j = 0; j < temp_size; j++) {
                if (temp[j] == s->twoHopSet[i].twoHopNeighborAddr) {
                    in = 1;
                }
            }
            
            if (!in) {
                temp[temp_size] = s->twoHopSet[i].twoHopNeighborAddr;
                temp_size++;
                assert(temp_size < OLSR_MAX_NEIGHBORS);
            }
        }
    }
    
    return temp_size;
}

/**
 * Event handler.  Basically covers two events at the moment:
 * - HELLO_TX: HELLO transmit required now, so package up all of our
 * neighbors into a message and send it.  Also schedule our next TX
 * - HELLO_RX: HELLO received, at the moment this only supports 1-hop
 * so we just pull the neighbor's address (neighbor_addrs[0]) from
 * the message.  HELLO_RX is dictated by HELLO_TX so don't schedule another
 *
 * TODO: Re-arrange logic so TX generates a single RX, which then schedules
 * another RX, which schedules another RX, etc. until all or informed
 */
void olsr_event(node_state *s, tw_bf *bf, olsr_msg_data *m, tw_lp *lp)
{
    int in;
    int i, j, k;
    hello *h;
    tw_event *e;
    tw_stime ts;
    olsr_msg_data *msg;
    //char covered[BITNSLOTS(OLSR_MAX_2_HOP)];
    
    switch(m->type) {
        case HELLO_TX:
            ts = tw_rand_exponential(lp->rng, HELLO_DELTA);
            
            tw_lp *cur_lp = g_tw_lp[0];
            
            e = tw_event_new(cur_lp->gid, ts, lp);
            msg = tw_event_data(e);
            msg->type = HELLO_RX;
            msg->originator = m->originator;
            msg->lng = s->lng;
            msg->lat = s->lat;
            msg->target = 0;
            h = &msg->mt.h;
            h->num_neighbors = s->num_neigh;// + 1;
            //h->neighbor_addrs[0] = s->local_address;
            for (j = 0; j < s->num_neigh; j++) {
                h->neighbor_addrs[j] = s->neighSet[j].neighborMainAddr;
            }
            tw_event_send(e);
            
            /*
            for (i = 0; i < g_tw_nlp; i++) {
                tw_stime ts = tw_rand_exponential(lp->rng, HELLO_DELTA);
                
                tw_lp *cur_lp = g_tw_lp[i];
                
                e = tw_event_new(cur_lp->gid, ts, lp);
                msg = tw_event_data(e);
                msg->type = HELLO_RX;
                msg->lng = s->lng;
                msg->lat = s->lat;
                h = &msg->mt.h;
                h->num_neighbors = s->num_neigh + 1;
                h->neighbor_addrs[0] = s->local_address;
                for (j = 0; j < s->num_neigh; j++) {
                    h->neighbor_addrs[j+1] = s->neighSet[j].neighborMainAddr;
                }
                tw_event_send(e);
            }
            */
            
            e = tw_event_new(lp->gid, HELLO_INTERVAL, lp);
            msg = tw_event_data(e);
            msg->type = HELLO_TX;
            msg->originator = s->local_address;
            msg->lng = s->lng;
            msg->lat = s->lat;
            h = &msg->mt.h;
            h->num_neighbors = 0;//1;
            //h->neighbor_addrs[0] = s->local_address;
            tw_event_send(e);
            
            break;
            
        case HELLO_RX:
            // TODO: Check if we can hear this message here?
            // Add neighbor_addrs[0] to 1-hop neighbor list
            
            h = &m->mt.h;
            
            in = 0;
            
            // If we receive our own message, don't add ourselves but
            // DO generate a new event for the next guy!
            
            // Copy the message we just received; we can't add data to
            // a message sent by another node
            if (m->target < g_tw_nlp - 1) {
                ts = tw_rand_exponential(lp->rng, HELLO_DELTA);
                
                tw_lp *cur_lp = g_tw_lp[m->target + 1];
                
                e = tw_event_new(cur_lp->gid, ts, lp);
                msg = tw_event_data(e);
                msg->type = HELLO_RX;
                msg->originator = m->originator;
                msg->lng = m->lng;
                msg->lat = m->lat;
                msg->target = m->target + 1;
                h = &msg->mt.h;
                h->num_neighbors = m->mt.h.num_neighbors;//m->num_neigh + 1;
                //h->neighbor_addrs[0] = s->local_address;
                for (j = 0; j < h->num_neighbors; j++) {
                    h->neighbor_addrs[j] = m->mt.h.neighbor_addrs[j];
                    //h->neighbor_addrs[j] = s->neighSet[j].neighborMainAddr;
                }
                tw_event_send(e);
            }
            
            // We've already passed along the message which has to happen
            // regardless of whether or not it can be heard, handled, etc.
            
            // Check to see if we can hear this message or not
            if (out_of_radio_range(s, m)) {
                //printf("Out of range!\n");
                return;
            }
            
            if (s->local_address == m->originator) {
                return;
            }
            
            // BEGIN 1-HOP PROCESSING
            for (i = 0; i < s->num_neigh; i++) {
                if (s->neighSet[i].neighborMainAddr == m->originator) {
                    in = 1;
                }
            }
            
            if (!in) {
                s->neighSet[s->num_neigh].neighborMainAddr = m->originator;
                s->num_neigh++;
                assert(s->num_neigh < OLSR_MAX_NEIGHBORS);
            }
            // END 1-HOP PROCESSING
            
            // BEGIN 2-HOP PROCESSING
            
            h = &m->mt.h;
            
            for (i = 0; i < h->num_neighbors; i++) {
                if (s->local_address == h->neighbor_addrs[i]) {
                    // We are not going to be our own 2-hop neighbor!
                    continue;
                }
                
                // Check and see if h->neighbor_addrs[i] is in our list
                // already
                in = 0;
                for (j = 0; j < s->num_two_hop; j++) {
                    if (s->twoHopSet[j].neighborMainAddr == m->originator &&
                        s->twoHopSet[j].twoHopNeighborAddr == h->neighbor_addrs[i]) {
                        in = 1;
                    }
                }
                
                if (!in) {
                    s->twoHopSet[s->num_two_hop].neighborMainAddr = m->originator;
                    s->twoHopSet[s->num_two_hop].twoHopNeighborAddr = h->neighbor_addrs[i];
                    assert(s->twoHopSet[s->num_two_hop].neighborMainAddr !=
                           s->twoHopSet[s->num_two_hop].twoHopNeighborAddr);
                    s->num_two_hop++;
                    assert(s->num_two_hop < OLSR_MAX_2_HOP);
                }
            }
            
            // END 2-HOP PROCESSING
            
            // BEGIN MPR COMPUTATION
            
            // Initially no nodes are covered
            memset(g_covered, 0, BITNSLOTS(OLSR_MAX_2_HOP));
            s->num_mpr = 0;
            
            // Copy all relevant information to scratch space
            g_num_one_hop = s->num_neigh;
            for (i = 0; i < g_num_one_hop; i++) {
                g_mpr_one_hop[i] = s->neighSet[i];
            }
            
            g_num_two_hop = s->num_two_hop;
            for (i = 0; i < g_num_two_hop; i++) {
                g_mpr_two_hop[i] = s->twoHopSet[i];
            }
            
            // Calculate D(y), where y is a member of N, for all nodes in N
            for (i = 0; i < g_num_one_hop; i++) {
                g_Dy[i] = Dy(s, g_mpr_one_hop[i].neighborMainAddr);
            }
            
            // Take care of the "unused" bits
            for (i = g_num_two_hop; i < OLSR_MAX_2_HOP; i++) {
                BITSET(g_covered, i);
            }
            
            // 3. Add to the MPR set those nodes in N, which are the *only*
            // nodes to provide reachability to a node in N2.
//            std::set<Ipv4Address> coveredTwoHopNeighbors;
//            for (TwoHopNeighborSet::const_iterator twoHopNeigh = N2.begin (); twoHopNeigh != N2.end (); twoHopNeigh++)
//            {
//                bool onlyOne = true;
//                // try to find another neighbor that can reach twoHopNeigh->twoHopNeighborAddr
//                for (TwoHopNeighborSet::const_iterator otherTwoHopNeigh = N2.begin (); otherTwoHopNeigh != N2.end (); otherTwoHopNeigh++)
//                {
//                    if (otherTwoHopNeigh->twoHopNeighborAddr == twoHopNeigh->twoHopNeighborAddr
//                        && otherTwoHopNeigh->neighborMainAddr != twoHopNeigh->neighborMainAddr)
//                    {
//                        onlyOne = false;
//                        break;
//                    }
//                }
//                if (onlyOne)
//                {
//                    NS_LOG_LOGIC ("Neighbor " << twoHopNeigh->neighborMainAddr
//                                  << " is the only that can reach 2-hop neigh. "
//                                  << twoHopNeigh->twoHopNeighborAddr
//                                  << " => select as MPR.");
//                    
//                    mprSet.insert (twoHopNeigh->neighborMainAddr);
//                    
//                    // take note of all the 2-hop neighbors reachable by the newly elected MPR
//                    for (TwoHopNeighborSet::const_iterator otherTwoHopNeigh = N2.begin ();
//                         otherTwoHopNeigh != N2.end (); otherTwoHopNeigh++)
//                    {
//                        if (otherTwoHopNeigh->neighborMainAddr == twoHopNeigh->neighborMainAddr)
//                        {
//                            coveredTwoHopNeighbors.insert (otherTwoHopNeigh->twoHopNeighborAddr);
//                        }
//                    }
//                }
//            }
            
            for (i = 0; i < g_num_two_hop; i++) {
                int onlyOne = 1;
                // try to find another neighbor that can reach twoHopNeigh->twoHopNeighborAddr
                for (j = 0; j < g_num_two_hop; j++) {
                    if (g_mpr_two_hop[j].twoHopNeighborAddr == g_mpr_two_hop[i].twoHopNeighborAddr
                        && g_mpr_two_hop[j].neighborMainAddr != g_mpr_two_hop[i].neighborMainAddr) {
                        onlyOne = 0;
                        break;
                    }
                }
                
                if (onlyOne) {
                    s->mprSet[s->num_mpr] = g_mpr_two_hop[i].neighborMainAddr;
                    s->num_mpr++;
                    
                    // take note of all the 2-hop neighbors reachable by the newly elected MPR
                    for (j = 0; j < g_num_two_hop; j++) {
                        if (g_mpr_two_hop[j].neighborMainAddr == g_mpr_two_hop[i].neighborMainAddr) {
                            //
                        }
                    }
                }
            }
            
            while (1) {
                int done = 1;
                
                for (i = 0; i < g_num_two_hop; i++) {
                    // If a node is not covered, we're not done!
                    if (!BITTEST(g_covered, i)) {
                        done = 0;
                    }
                }
                
                if (done) break;
                
                for (i = 0; i < g_num_two_hop; i++) {
                    if (BITTEST(g_covered, i)) {
                        // If node _i_ is covered, continue;
                        continue;
                    }
                    
                    // Node _i_ is NOT covered, check if there exists a two-hop
                    // neighbor that is equivalent to i
                    tw_lp *t = g_tw_lp[i];
                    
                    int count = 0;
                    
                    for (j = 0; j < g_num_two_hop; j++) {
                        if (g_mpr_two_hop[j].twoHopNeighborAddr == t->gid) {
                            g_mpr_neigh_to_add = g_mpr_two_hop[j];
                            count++;
                            printf("count is %d\n", count);
                        }
                    }
                    
                    if (count == 1) {
                        // Only 1 path exists!  We have to add this node-pair
                        s->mprSet[s->num_mpr] = g_mpr_neigh_to_add.neighborMainAddr;
                        s->num_mpr++;
                        // Remove the nodes from N2 which are now covered by a
                        // node in the MPR set
                        for (j = 0; j < g_num_two_hop; j++) {
                            if (g_mpr_two_hop[j].neighborMainAddr == s->mprSet[s->num_mpr-1]) {
                                for (k = 0; k < OLSR_MAX_2_HOP; k++) {
                                    if (
                                }
                            }
                        }
                    }
                }
            }
            
            // END MPR COMPUTATION
            
            break;
            
            /*
        case HELLO:
            // TODO: Add new nodes
            // ...
            
            // Schedule next event
            e = tw_event_new(lp->gid, HELLO_INTERVAL, lp);
            msg = tw_event_data(e);
            msg->type = HELLO;
            msg->node_id = lp->gid;
            msg->lng = s->lng;
            msg->lat = s->lat;
            // Should this also happen for num_tuples or what?
            for (i = 0; i < s->num_neigh; i++) {
                msg->mt.h.neighbor_addrs[i] = s->neighSet[i].neighborMainAddr;
            }
            msg->mt.h.num_neighbors = s->num_neigh;
            tw_event_send(e);
             */
    }
}

void olsr_final(node_state *s, tw_lp *lp)
{
    int i;
    
    printf("node %lu contains %d neighbors\n", s->local_address, s->num_neigh);
    for (i = 0; i < s->num_neigh; i++) {
        printf("   neighbor[%d] is %lu\n", i, s->neighSet[i].neighborMainAddr);
        printf("   Dy(%lu) is %d\n", s->neighSet[i].neighborMainAddr,
               Dy(s, s->neighSet[i].neighborMainAddr));
    }
    printf("node %lu has %d two-hop neighbors\n", s->local_address, 
           s->num_two_hop);
    for (i = 0; i < s->num_two_hop; i++) {
        printf("   two-hop neighbor[%d] is %lu : %lu\n", i, 
               s->twoHopSet[i].neighborMainAddr,
               s->twoHopSet[i].twoHopNeighborAddr);
    }
}

tw_peid olsr_map(tw_lpid gid)
{
    return (tw_peid)gid / g_tw_nlp;
}

tw_lptype olsr_lps[] = {
    {
        (init_f) olsr_init,
        (event_f) olsr_event,
        (revent_f) NULL,
        (final_f) olsr_final,
        (map_f) olsr_map,
        sizeof(node_state)
    },
    { 0 },
};

const tw_optdef olsr_opts[] = {
    TWOPT_GROUP("OLSR Model"),
    TWOPT_UINT("lp_per_pe", nlp_per_pe, "number of LPs per processor"),
    TWOPT_END(),
};

// Done mainly so doxygen will catch and differentiate this main
// from other mains while allowing smooth compilation.
#define olsr_main main

int olsr_main(int argc, char *argv[])
{
    int i;
    
    g_tw_events_per_pe = nlp_per_pe * nlp_per_pe * 1;
    
    tw_opt_add(olsr_opts);
    
    tw_init(&argc, &argv);
    
    tw_define_lps(nlp_per_pe, sizeof(olsr_msg_data), 0);
    
    for (i = 0; i < g_tw_nlp; i++) {
        tw_lp_settype(i, &olsr_lps[0]);
    }
    
    tw_run();
    
    if (tw_ismaster()) {
        printf("Complete.\n");
    }
    
    tw_end();
    
    return 0;
}