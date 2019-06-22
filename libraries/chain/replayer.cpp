
namespace weku{namespace chain{

// TODO: need to improve the performance of this function, since it's quite slow now.
// it's been called when user run: wekud --replay
void replayer::reindex( const fc::path& data_dir, const fc::path& shared_mem_dir, uint64_t shared_file_size )
{
   try
   {
      ilog( "Reindexing Blockchain" );
      // wipe here is delete two files: shared_memory.bin and shared_memory.meta
      // but keep block_log and block_log.index file.
      _db.wipe( data_dir, shared_mem_dir, false );
      // fix reindex bug here
      //open( data_dir, shared_mem_dir, 0, shared_file_size, chainbase::database::read_write );
      _db.open( data_dir, shared_mem_dir, STEEMIT_INIT_SUPPLY, shared_file_size, chainbase::database::read_write );
      _db.fork_db().reset();    // override effect of _fork_db.start_block() call in open()

      auto start = fc::time_point::now();
      STEEMIT_ASSERT( _block_log.head(), block_log_exception, "No blocks in block log. Cannot reindex an empty chain." );

      ilog( "Replaying blocks..." );

      uint64_t skip_flags =
         skip_witness_signature |
         skip_transaction_signatures |
         skip_transaction_dupe_check |
         skip_tapos_check |
         skip_merkle_check |
         skip_witness_schedule_check |
         skip_authority_check |
         skip_validate | /// no need to validate operations
         skip_validate_invariants |
         skip_block_log;

      _db.with_write_lock( [&]()
      {
          // here 0 is file position, not block number, so itr will point to first block: block #1
         auto itr = _db.get_block_log.read_block( 0 );
         auto last_block_num = _db.get_block_log.head()->block_num();

         while( itr.first.block_num() != last_block_num )
         {
            auto cur_block_num = itr.first.block_num();
            if( cur_block_num % 100000 == 0 ) // the free memory here means free disk space of mapping file, not physical memory.
               std::cerr << "   " << double( cur_block_num * 100 ) / last_block_num << "%   " << cur_block_num << " of " << last_block_num <<
               "   (" << (_db.get_free_memory() / (1024*1024)) << "M free)\n";
            _db.apply_block( itr.first, skip_flags );
            itr = _db.get_block_log.read_block( itr.second ); // itr.second points to the position of next block.
         }

         _db.apply_block( itr.first, skip_flags ); // apply last_block_num/head block number
          // set revision to head block number,
          // which in turn set all revisions to head_block_num of indices/tables contained in current database.
         _db.set_revision( head_block_num() );
      });

      if( _db.get_block_log.head()->block_num() )
         _db.fork_db().start_block( *_db.get_block_log.head() );

      auto end = fc::time_point::now();
      ilog( "Done reindexing, elapsed time: ${t} sec", ("t",double((end-start).count())/1000000.0 ) );
   }
   FC_CAPTURE_AND_RETHROW( (data_dir)(shared_mem_dir) )

}

}}