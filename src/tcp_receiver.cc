#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  // debug( "unimplemented receive() called" );
  if ( message.RST ) {
    reader().set_error();
    return;
  }

  if (!message.SYN && !ISN_inited) {
    return;
  }

  if ( message.SYN ) {
    if ( ISN_inited ) {
      return;
    }

    zero_point = message.seqno;
    ISN_inited = true;
  }

  checkpoint = 1 + writer().bytes_pushed();
  uint64_t first_index = message.seqno.unwrap( zero_point, checkpoint );
  if ( !message.SYN && first_index == 0 ) {
    if ( message.payload.empty() ) {
      return;
    }

    reassembler_.insert( 0, message.payload.substr( 1 ), message.FIN );
    return;
  }

  uint64_t stream_index = first_index - 1 + ( message.SYN ? 1 : 0 );
  bool is_last = message.FIN;
  reassembler_.insert( stream_index, message.payload, is_last );
}

// send current TCPReceiver status
TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  // debug( "unimplemented send() called" );
  TCPReceiverMessage msg;
  if ( !ISN_inited ) {
    msg.ackno = {};
  } else {
    uint64_t abs_ackno = 1 + writer().bytes_pushed() + writer().is_closed();
    msg.ackno = Wrap32::wrap( abs_ackno, zero_point );
  }
  msg.RST = reader().has_error();
  msg.window_size = std::min(writer().available_capacity(), (uint64_t)UINT16_MAX);
  return msg;
}
