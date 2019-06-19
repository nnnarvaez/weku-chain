#pragma once
#include <weku/protocol/block_header.hpp>
#include <weku/protocol/transaction.hpp>

namespace weku { namespace protocol {

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root()const;
      vector<signed_transaction> transactions;
   };

} } // weku::protocol

FC_REFLECT_DERIVED( weku::protocol::signed_block, (weku::protocol::signed_block_header), (transactions) )
