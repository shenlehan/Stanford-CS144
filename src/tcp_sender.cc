#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// How many sequence numbers are outstanding?
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // debug( "unimplemented sequence_numbers_in_flight() called" );
  return sequence_numbers_in_flight_;
}

// How many consecutive retransmissions have happened?
uint64_t TCPSender::consecutive_retransmissions() const
{
  // debug( "unimplemented consecutive_retransmissions() called" );
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // debug( "unimplemented push() called" );
  // (void)transmit;

  size_t window_size = recv_windows_size_ == 0 ? 1 : recv_windows_size_;

  while ( window_size > sequence_numbers_in_flight_ ) {
    TCPSenderMessage msg;
    uint64_t available_space = window_size - sequence_numbers_in_flight_;

    if ( !SYN_inited_ ) {
      msg.SYN = true;
      SYN_inited_ = true;
    }

    std::string_view const data = reader().peek();
    uint64_t const max_payload = std::min( TCPConfig::MAX_PAYLOAD_SIZE, available_space - msg.sequence_length() );
    if ( !data.empty() && max_payload > 0 ) {
      size_t const actual_size = std::min( data.size(), max_payload );
      msg.payload = std::string( data.substr( 0, actual_size ) );
      input_.reader().pop( actual_size );
    }

    if ( !FIN_inited_ && writer().is_closed() && msg.sequence_length() < available_space
         && reader().bytes_buffered() == 0 ) {
      msg.FIN = true;
      FIN_inited_ = true;
    }

    if ( msg.sequence_length() == 0 ) {
      break;
    }

    if (writer().has_error()) {
      msg.RST = true;
    }

    sequence_numbers_in_flight_ += msg.sequence_length();
    msg.seqno = Wrap32::wrap( next_seqno_, isn_ );
    transmit( msg );
    outstanding_segments_.push_back( msg );
    if ( !timer_running_ ) {
      timer_running_ = true;
      timer_ms_ = 0;
    }

    next_seqno_ = next_seqno_ + static_cast<uint32_t>( msg.sequence_length() );
    if ( recv_windows_size_ == 0 ) {
      break;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // debug( "unimplemented make_empty_message() called" );
  TCPSenderMessage msg;
  msg.FIN = false;
  msg.SYN = false;
  msg.seqno = Wrap32::wrap( next_seqno_, isn_ );
  if (writer().has_error()) {
    msg.RST = true;
  }
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // debug( "unimplemented receive() called" );
  if ( msg.RST ) { // error
    writer().set_error();
    return;
  }
  recv_windows_size_ = msg.window_size; // new window size
  if ( !msg.ackno.has_value() ) {
    return;
  }

  uint64_t msg_ackno_unwrap = msg.ackno->unwrap( isn_, next_seqno_ );
  if ( msg_ackno_unwrap > next_seqno_ ) { // ackno shouldn't be that large~
    return;
  }

  bool has_new_ackno = false;
  if ( msg_ackno_unwrap > last_ackno_ ) {
    has_new_ackno = true;
  }

  last_ackno_ = msg_ackno_unwrap > last_ackno_ ? msg_ackno_unwrap : last_ackno_;
  while ( !outstanding_segments_.empty() ) {
    TCPSenderMessage& seg = outstanding_segments_.front();

    uint64_t seg_start = seg.seqno.unwrap( isn_, msg_ackno_unwrap );
    uint64_t seg_end = seg_start + seg.sequence_length();

    if ( seg_end <= last_ackno_ ) {
      sequence_numbers_in_flight_ -= seg.sequence_length();
      outstanding_segments_.pop_front();
    } else {
      break;
    }
  }

  if ( has_new_ackno ) {
    current_RTO_ = initial_RTO_ms_;
    consecutive_retransmissions_ = 0;
    if ( !outstanding_segments_.empty() ) {
      timer_ms_ = 0;
    } else {
      timer_ms_ = 0;
      timer_running_ = false;
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // debug( "unimplemented tick({}, ...) called", ms_since_last_tick );
  // (void)transmit;
  timer_ms_ += ms_since_last_tick;
  if ( timer_running_ && timer_ms_ >= current_RTO_ ) {
    transmit( outstanding_segments_.front() );
    if ( recv_windows_size_ > 0 ) {
      consecutive_retransmissions_ += 1;
      current_RTO_ *= 2;
    }
    timer_ms_ = 0;
  }
}
