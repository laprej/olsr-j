#ifndef OLSR_H_
#define OLSR_H_

/**
 * @file
 * @brief OLSR structs, constants, etc.
 * 
 * We're going to try really hard to mimic ns3 as much as possible in this
 * implementation.  We will assume the following:
 * - strictly symmetric links
 * - single interface
 */

#include "ross.h"

/** max neighbors (for array implementation) */
#define OLSR_MAX_NEIGHBORS 30

typedef tw_lpid o_addr; /**< We'll use this as a place holder for addresses */
typedef double Time;    /**< Use a double for time, check w/ Chris */

/**
 struct hello - a basic hello message used by OLSR for link sensing / topology
 detection.
 
 Here is the ns3 hello class:
 @code
struct Hello
{
    struct LinkMessage {
        uint8_t linkCode;
        std::vector<Ipv4Address> neighborInterfaceAddresses;
    };
    
    uint8_t hTime;
    void SetHTime (Time time)
    {
        this->hTime = SecondsToEmf (time.GetSeconds ());
    }
    Time GetHTime () const
    {
        return Seconds (EmfToSeconds (this->hTime));
    }
    
    uint8_t willingness;
    std::vector<LinkMessage> linkMessages;
    
    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator start) const;
    uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
};
 @endcode
 */

typedef struct hello {
    /* No support for link codes yet! */
    /** Addresses of our neighbors */
    o_addr neighbor_addrs[OLSR_MAX_NEIGHBORS];
    /** Number of neighbors, 0..n-1 */
    unsigned num_neighbors;
    
    /** HELLO emission interval */
    uint8_t hTime;
    
    /** Willingness to carry and foward traffic for other nodes */
    uint8_t Willingness;
    /* Link message size is an unnecessary field */
} hello_t;

struct LinkTuple
{
    /// Interface address of the local node.
    o_addr localIfaceAddr;
    /// Interface address of the neighbor node.
    o_addr neighborIfaceAddr;
    /// The link is considered bidirectional until this time.
    Time symTime;
    /// The link is considered unidirectional until this time.
    Time asymTime;
    /// Time at which this tuple expires and must be removed.
    Time time;
};

struct NeighborTuple
{
    /// Main address of a neighbor node.
    o_addr neighborMainAddr;
    /// Neighbor Type and Link Type at the four less significative digits.
    enum Status {
        STATUS_NOT_SYM = 0, // "not symmetric"
        STATUS_SYM = 1, // "symmetric"
    } status;
    /// A value between 0 and 7 specifying the node's willingness to carry traffic on behalf of other nodes.
    uint8_t willingness;
};

struct TwoHopNeighborTuple
{
    /// Main address of a neighbor.
    o_addr neighborMainAddr;
    /// Main address of a 2-hop neighbor with a symmetric link to nb_main_addr.
    o_addr twoHopNeighborAddr;
    /// Time at which this tuple expires and must be removed.
    Time expirationTime; // previously called 'time_'
};

/**
 This struct contains all of the OLSR per-node state.  Not everything in the
 ns3 class is necessary or implemented, but here is the ns3 OlsrState class:
 @code
class OlsrState
{
    //  friend class Olsr;
    
protected:
    LinkSet m_linkSet;    ///< Link Set (RFC 3626, section 4.2.1).
    NeighborSet m_neighborSet;            ///< Neighbor Set (RFC 3626, section 4.3.1).
    TwoHopNeighborSet m_twoHopNeighborSet;        ///< 2-hop Neighbor Set (RFC 3626, section 4.3.2).
    TopologySet m_topologySet;    ///< Topology Set (RFC 3626, section 4.4).
    MprSet m_mprSet;      ///< MPR Set (RFC 3626, section 4.3.3).
    MprSelectorSet m_mprSelectorSet;      ///< MPR Selector Set (RFC 3626, section 4.3.4).
    DuplicateSet m_duplicateSet;  ///< Duplicate Set (RFC 3626, section 3.4).
    IfaceAssocSet m_ifaceAssocSet;        ///< Interface Association Set (RFC 3626, section 4.1).
    AssociationSet m_associationSet; ///<	Association Set (RFC 3626, section12.2). Associations obtained from HNA messages generated by other nodes.
    Associations m_associations;  ///< The node's local Host Network Associations that will be advertised using HNA messages.
};
 @endcode
 */

typedef struct olsr_node_state {
    
} node_state_t;


#endif /* OLSR_H_ */
