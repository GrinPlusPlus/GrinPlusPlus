## Grin++ Database

The database project consists of a minimal, light-weight wrapper around RocksDB. It was designed in a way that allows developers to write their own implementations using any database implementation out there, from lightweight embedded key-value DBs to large-scale Redis or Memcached clusters.

Custom database implementations will be drop-in replacements as long as they implement the interfaces and exports defined in `include/Database`.