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

  if (message.SYN) {
    if (ISN_inited) {
      return;
    }

    zero_point = message.seqno;
    ISN_inited = true;
  }

  checkpoint = reassembler_.get_first_unassembled_idx();
  uint64_t first_index = message.seqno.unwrap( zero_point, checkpoint );
  uint64_t stream_index = first_index - 1 + (message.SYN ? 1 : 0);
  bool is_last = message.FIN;
  reassembler_.insert( stream_index, message.payload, is_last );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  // debug( "unimplemented send() called" );

  return {};
}
