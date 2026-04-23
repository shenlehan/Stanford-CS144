#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  // cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
  //      << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
  //      << " on interface " << interface_num << "\n";
  //
  // debug( "unimplemented add_route() called" );

  uint32_t index;
  uint32_t mask;
  if ( prefix_length == 0 ) {
    mask = 0;
  } else if ( prefix_length == 32 ) {
    mask = UINT32_MAX;
  } else {
    mask = ( ( ( 1 << prefix_length ) - 1 ) << ( 32 - prefix_length ) );
  }

  index = route_prefix & mask;
  router_table_.insert_or_assign( index,
                                  RouterTableEntry { route_prefix, prefix_length, next_hop, interface_num } );
}

std::optional<uint32_t> Router::get_lpm( const InternetDatagram& dgram )
{
  uint32_t dst_ip = dgram.header.dst;
  uint32_t max_prefix_length = 0;
  std::optional<uint32_t> match_rte_idx;
  for ( auto& [router_prefix, rte] : router_table_ ) {
    if ( rte.prefix_length != 0 ) {
      if ( ( router_prefix >> ( 32 - rte.prefix_length ) ) == ( dst_ip >> ( 32 - rte.prefix_length ) ) ) {
        if ( rte.prefix_length > max_prefix_length ) {
          match_rte_idx = router_prefix;
          max_prefix_length = rte.prefix_length;
        }
      }
    } else {
      if ( rte.prefix_length >= max_prefix_length ) {
        match_rte_idx = router_prefix;
        max_prefix_length = rte.prefix_length;
      }
    }
  }

  return match_rte_idx;
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // debug( "unimplemented route() called" );
  for ( auto interface : interfaces_ ) {
    auto& q = interface->datagrams_received();
    while ( !q.empty() ) {
      auto dgram = q.front();
      q.pop();

      std::optional<uint32_t> match_rte_idx = get_lpm( dgram );
      if ( !match_rte_idx.has_value() ) {
        continue;
      }

      auto it = router_table_.find( match_rte_idx.value() );
      if ( it == router_table_.end() ) {
        continue;
      }
      RouterTableEntry& rte = it->second;

      if ( dgram.header.ttl <= 1 ) { // TTL runs out, drop it
        continue;
      }
      dgram.header.ttl -= 1;
      dgram.header.compute_checksum();

      if ( rte.next_hop.has_value() ) {
        interfaces_[rte.interface_num]->send_datagram( dgram, rte.next_hop.value() );
      } else {
        interfaces_[rte.interface_num]->send_datagram( dgram, Address::from_ipv4_numeric( dgram.header.dst ) );
      }
    }
  }
}
