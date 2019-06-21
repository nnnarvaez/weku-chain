// #pragma once
// #include <weku/chain/database.hpp>

// namespace weku{namespace chain{

// class mock_database: public database
// {
// public:
//     virtual void init_genesis(uint64_t initial_supply = 0) override;
//     virtual uint32_t head_block_num() const override; 

//     virtual uint32_t last_hardfork() const  override{return _last_hardfork;};  
//     virtual void last_hardfork(uint32_t hardfork)  override{_last_hardfork = hardfork;} // TODO: need to encapsulate it.
//     virtual const hardfork_votes_type& next_hardfork_votes() const override;  
    
//     virtual bool apply_block(const signed_block& b, uint32_t skip = 0) override;
    
//     virtual void reindex( const fc::path& data_dir, const fc::path& shared_mem_dir, uint64_t shared_file_size = (1024l*1024l*1024l*8l) ) override;
    
//     virtual signed_block generate_block_before_apply() override; // TODO
//     virtual signed_block generate_block() override; // TOTO: need refactory this interfac, maybe debug_generate_block?
//     virtual signed_block debug_generate_block_until(uint32_t block_num) override; // TODO:
//     virtual void debug_set_next_hardfork_votes(hardfork_votes_type next_hardfork_votes) override; // TODO:
// private:
//     uint32_t _head_block_num = 0;
//     uint32_t _last_hardfork = 0;
//     std::vector<signed_block> _ledger_blocks;
//     hardfork_votes_type _next_hardfork_votes; // TODO: move to a dependent object, and extract data from shuffled witnesses
// };

// }}