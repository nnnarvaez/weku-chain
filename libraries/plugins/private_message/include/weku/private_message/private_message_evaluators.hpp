#pragma once

#include <weku/chain/evaluator.hpp>

#include <weku/private_message/private_message_operations.hpp>
#include <weku/private_message/private_message_plugin.hpp>

namespace weku { namespace private_message {

DEFINE_PLUGIN_EVALUATOR( private_message_plugin, weku::private_message::private_message_plugin_operation, private_message )

} }
