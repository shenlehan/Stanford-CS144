#pragma once

#include "byte_stream.hh"
#include <utility>
#include <vector>

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output )
    : output_( std::move( output ) )
    , first_unpopped_idx( 0 )
    , first_unassembled_idx( 0 )
    , first_unaccepted_idx( output_.writer().available_capacity() )
    , buffer_( output_.writer().available_capacity(), '\0' )
    , bitmap_( output_.writer().available_capacity(), false )
    , buffer_capacity_( output_.writer().available_capacity() )
    , eof_idx_( 0 )
    , has_eof_( false )
  {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, const std::string& data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  // This function is for testing only; don't add extra state to support it.
  uint64_t count_bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }
  uint64_t get_first_unassembled_idx() { return first_unassembled_idx; }

private:
  ByteStream output_;
  uint64_t first_unpopped_idx { 0 };
  uint64_t first_unassembled_idx;
  uint64_t first_unaccepted_idx;
  std::string buffer_;
  std::vector<bool> bitmap_;
  uint64_t buffer_capacity_;
  void try_push();
  uint64_t eof_idx_;
  bool has_eof_;
};
