#include <weku/witness/witness_operations.hpp>

#include <weku/protocol/operation_util_impl.hpp>

namespace weku { namespace witness {

void enable_content_editing_operation::validate()const
{
   chain::validate_account_name( account );
}

} } // weku::witness

DEFINE_OPERATION_TYPE( weku::witness::witness_plugin_operation )
