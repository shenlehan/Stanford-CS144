#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <functional>
#include <list>
#include <queue>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) )
    , isn_( isn )
    , initial_RTO_ms_( initial_RTO_ms )
    , recv_windows_size_( 1 )
    , sequence_numbers_in_flight_( 0 )
    , SYN_inited_( false )
    , FIN_inited_( false )
    , next_seqno_( 0 )
    , outstanding_segments_()
    , timer_running_( false )
    , timer_ms_( 0 )
    , current_RTO_( initial_RTO_ms )
    , consecutive_retransmissions_( 0 )
    , last_ackno_ {}
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t recv_windows_size_;
  uint64_t sequence_numbers_in_flight_;
  bool SYN_inited_;
  bool FIN_inited_;
  uint64_t next_seqno_;
  std::list<TCPSenderMessage> outstanding_segments_;
  bool timer_running_;
  uint64_t timer_ms_;
  uint64_t current_RTO_;
  uint64_t consecutive_retransmissions_;
  std::optional<uint64_t> last_ackno_;
};
