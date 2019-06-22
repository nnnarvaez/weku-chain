#include <weku/follow/follow_operations.hpp>

#include <weku/protocol/operation_util_impl.hpp>

namespace weku { namespace follow {

void follow_operation::validate()const
{
   FC_ASSERT( follower != following, "You cannot follow yourself" );
}

void reblog_operation::validate()const
{
   FC_ASSERT( account != author, "You cannot reblog your own content" );
}

} } //weku::follow

DEFINE_OPERATION_TYPE( weku::follow::follow_plugin_operation )
