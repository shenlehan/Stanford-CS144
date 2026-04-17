#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  // debug( "unimplemented wrap( {}, {} ) called", n, zero_point.raw_value_ );
  return Wrap32 { zero_point + static_cast<uint32_t>( n ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  // debug( "unimplemented unwrap( {}, {} ) called", zero_point.raw_value_, checkpoint );
  uint64_t val = raw_value_ - zero_point.raw_value_;
  uint64_t candidate = (checkpoint & 0xFFFFFFFF00000000ULL) + val;
  if ( candidate > checkpoint + ( 1ULL << 31 ) && candidate >= ( 1ULL << 32 ) ) {
    return candidate - ( 1ULL << 32 );
  }
  if ( checkpoint > candidate + ( 1ULL << 31 ) ) {
    return candidate + ( 1ULL << 32 );
  }
  return candidate;
}
