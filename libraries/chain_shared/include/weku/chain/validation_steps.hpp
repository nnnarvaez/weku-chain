namespace weku{namespace chain{
    enum validation_steps
    {
    skip_nothing                = 0,
    skip_witness_signature      = 1 << 0,  ///< used while reindexing
    skip_transaction_signatures = 1 << 1,  ///< used by non-witness nodes
    skip_transaction_dupe_check = 1 << 2,  ///< used while reindexing
    skip_fork_db                = 1 << 3,  ///< used while reindexing
    skip_block_size_check       = 1 << 4,  ///< used when applying locally generated transactions
    skip_tapos_check            = 1 << 5,  ///< used while reindexing -- note this skips expiration check as well
    skip_authority_check        = 1 << 6,  ///< used while reindexing -- disables any checking of authority on transactions
    skip_merkle_check           = 1 << 7,  ///< used while reindexing
    skip_undo_history_check     = 1 << 8,  ///< used while reindexing
    skip_witness_schedule_check = 1 << 9,  ///< used while reindexing
    skip_validate               = 1 << 10, ///< used prior to checkpoint, skips validate() call on transaction
    skip_validate_invariants    = 1 << 11, ///< used to skip database invariant check on block application
    skip_undo_block             = 1 << 12, ///< used to skip undo db on reindex
    skip_block_log              = 1 << 13  ///< used to skip block logging on reindex
};
}}