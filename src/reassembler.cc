#include "reassembler.hh"
#include "debug.hh"
#include <algorithm>

using namespace std;

void Reassembler::try_push()
{
  uint64_t pushed = 0;
  while ( pushed < buffer_.size() && bitmap_[pushed] ) {
    string one_byte( 1, buffer_[pushed] );
    output_.writer().push( one_byte );
    ++pushed;
  }

  if ( pushed == 0 ) {
    return;
  }

  // move the buffer
  for ( uint64_t j = 0; j + pushed < buffer_.size(); ++j ) {
    buffer_[j] = buffer_[j + pushed];
    bitmap_[j] = bitmap_[j + pushed];
  }
  for ( uint64_t j = buffer_.size() - pushed; j < buffer_.size(); ++j ) {
    bitmap_[j] = false;
    buffer_[j] = '\0';
  }
  first_unassembled_idx += pushed;
}

void Reassembler::insert( uint64_t first_index, const string& data, bool is_last_substring )
{
  // debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
  first_unpopped_idx = first_unassembled_idx - output_.reader().bytes_buffered();
  first_unaccepted_idx = first_unpopped_idx + buffer_capacity_;
  uint64_t start_idx = std::max( first_index, first_unassembled_idx );
  uint64_t last_idx = std::min( first_index + data.size(), first_unaccepted_idx );
  if (is_last_substring) {
    eof_idx_ = first_index + data.size();
    has_eof_ = true;
  }

  // can write
  if ( start_idx < last_idx ) {
    uint64_t len = last_idx - start_idx;
    for ( uint64_t i = 0; i < len; ++i ) {
      buffer_[start_idx - first_unassembled_idx + i] = data[start_idx - first_index + i];
      bitmap_[start_idx - first_unassembled_idx + i] = true;
    }
    try_push();
  }

  if ( has_eof_ && first_unassembled_idx == eof_idx_ ) {
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  // debug( "unimplemented count_bytes_pending() called" );
  return std::count( bitmap_.begin(), bitmap_.end(), true );
}
