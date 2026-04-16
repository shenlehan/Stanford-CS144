#include "byte_stream.hh"
#include "debug.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer_() {}

// Push data to stream, but only as much as available capacity allows.
void Writer::push( const string& data )
{
  // Your code here (and in each method below)
  // debug( "Writer::push({}) not yet implemented", data );
  if (data.length() + buffer_.length() <= capacity_) {
    buffer_ += data;
    pushed_ += data.length();
  } else {
    pushed_ += capacity_ - buffer_.length();
    buffer_ += data.substr( 0, capacity_ - buffer_.length());
  }
}

// Signal that the stream has reached its ending. Nothing more will be written.
void Writer::close()
{
  // debug( "Writer::close() not yet implemented" );
  closed_ = true;
}

// Has the stream been closed?
bool Writer::is_closed() const
{
  // debug( "Writer::is_closed() not yet implemented" );
  return closed_; // Your code here.
}

// How many bytes can be pushed to the stream right now?
uint64_t Writer::available_capacity() const
{
  // debug( "Writer::available_capacity() not yet implemented" );
  return capacity_ - buffer_.length(); // Your code here.
}

// Total number of bytes cumulatively pushed to the stream
uint64_t Writer::bytes_pushed() const
{
  // debug( "Writer::bytes_pushed() not yet implemented" );
  return pushed_; // Your code here.
}

// Peek at the next bytes in the buffer -- ideally as many as possible.
// It's not required to return a string_view of the *whole* buffer, but
// if the peeked string_view is only one byte at a time, it will probably force
// the caller to do a lot of extra work.
string_view Reader::peek() const
{
  // debug( "Reader::peek() not yet implemented" );
  return std::string_view( buffer_ ); // Your code here.
}

// Remove `len` bytes from the buffer.
void Reader::pop( uint64_t len )
{
  // debug( "Reader::pop({}) not yet implemented", len );
  if (len <= buffer_.length()) {
    buffer_.erase( 0, len );
    popped_ += len;
  } else {
    popped_ += buffer_.length();
    buffer_.clear();
  }
}

// Is the stream finished (closed and fully popped)?
bool Reader::is_finished() const
{
  // debug( "Reader::is_finished() not yet implemented" );
  return buffer_.empty() && closed_; // Your code here.
}

// Number of bytes currently buffered (pushed and not popped)
uint64_t Reader::bytes_buffered() const
{
  // debug( "Reader::bytes_buffered() not yet implemented" );
  return buffer_.length(); // Your code here.
}

// Total number of bytes cumulatively popped from stream
uint64_t Reader::bytes_popped() const
{
  // debug( "Reader::bytes_popped() not yet implemented" );
  return popped_; // Your code here.
}
