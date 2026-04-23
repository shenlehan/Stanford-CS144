#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
  , ms_tick_( 0 )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( InternetDatagram dgram, const Address& next_hop )
{
  uint32_t const ip_nxt_hop = next_hop.ipv4_numeric();
  uint64_t const now = ms_tick_;
  if ( ip_to_ether_[ip_nxt_hop].has_value() && now - ip_to_ether_[ip_nxt_hop].value().learned_at_ms < 30000 ) {
    // has next hop
    EthernetHeader header {};
    header.type = EthernetHeader::TYPE_IPv4;
    header.src = ethernet_address_;
    header.dst = ip_to_ether_[ip_nxt_hop].value().ethernet_address;

    EthernetFrame frame;
    frame.header = header;
    frame.payload = serialize( dgram );
    transmit( frame );
  } else {
    // broadcast to get ethernet address
    EthernetHeader header {};
    header.type = EthernetHeader::TYPE_ARP;
    header.src = ethernet_address_;
    header.dst = ETHERNET_BROADCAST;

    ARPMessage msg;
    msg.opcode = ARPMessage::OPCODE_REQUEST;
    msg.sender_ip_address = ip_address_.ipv4_numeric();
    msg.target_ip_address = ip_nxt_hop;
    msg.sender_ethernet_address = ethernet_address_;

    EthernetFrame frame;
    frame.header = header;
    frame.payload = serialize( msg );

    if ( !last_arp_request_.contains( ip_nxt_hop ) || now - last_arp_request_[ip_nxt_hop] >= 5000 ) {
      while ( !datagrams_queue_[ip_nxt_hop].empty() ) { // clear outdated packets
        datagrams_queue_[ip_nxt_hop].pop();
      }
      last_arp_request_[ip_nxt_hop] = now;
      transmit( frame ); // ask for new ethernet address
      datagrams_queue_[ip_nxt_hop].push( dgram );
    } else if ( now - last_arp_request_[ip_nxt_hop] < 5000 ) { // our ethernet address is very new
      datagrams_queue_[ip_nxt_hop].push( dgram );
    }
  }
}

void NetworkInterface::flush( uint32_t sender_ip, EthernetAddress sender_ether )
{
  while ( !datagrams_queue_[sender_ip].empty() ) {
    InternetDatagram dgram = datagrams_queue_[sender_ip].front();
    datagrams_queue_[sender_ip].pop();

    if (ms_tick_ - last_arp_request_[sender_ip] > 5000) {
      continue;
    }

    EthernetFrame ethernet_frame;
    ethernet_frame.header.type = EthernetHeader::TYPE_IPv4;
    ethernet_frame.header.src = ethernet_address_;
    ethernet_frame.header.dst = sender_ether;
    ethernet_frame.payload = serialize( dgram );
    transmit( ethernet_frame );
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  // debug( "unimplemented recv_frame called" );
  // (void)frame;
  EthernetAddress const address = frame.header.dst;
  if ( address != ethernet_address_ && address != ETHERNET_BROADCAST ) {
    return;
  }

  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    Parser parser( frame.payload );
    ARPMessage arp;
    arp.parse( parser );
    if ( parser.has_error() || !arp.supported() ) {
      return;
    }
    uint32_t const sender_ip = arp.sender_ip_address;
    EthernetAddress const sender_ether = arp.sender_ethernet_address;
    ip_to_ether_[sender_ip] = { .ethernet_address = sender_ether, .learned_at_ms = ms_tick_ };

    if ( arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == ip_address_.ipv4_numeric() ) {
      ARPMessage reply;
      reply.opcode = ARPMessage::OPCODE_REPLY;
      reply.sender_ethernet_address = ethernet_address_;
      reply.sender_ip_address = ip_address_.ipv4_numeric();
      reply.target_ethernet_address = sender_ether;
      reply.target_ip_address = sender_ip;

      EthernetFrame ethernet_frame;
      ethernet_frame.header.type = EthernetHeader::TYPE_ARP;
      ethernet_frame.header.src = ethernet_address_;
      ethernet_frame.header.dst = sender_ether;
      ethernet_frame.payload = serialize( reply );
      transmit( ethernet_frame );

    } else if ( arp.opcode == ARPMessage::OPCODE_REPLY && arp.target_ip_address == ip_address_.ipv4_numeric() ) {
    }

    flush( sender_ip, sender_ether );

  } else if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    Parser parser( frame.payload );
    InternetDatagram dgram;
    dgram.parse( parser );
    if ( parser.has_error() ) {
      return;
    }
    datagrams_received_.push( dgram );
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // debug( "unimplemented tick({}) called", ms_since_last_tick );
  ms_tick_ += ms_since_last_tick;
}
