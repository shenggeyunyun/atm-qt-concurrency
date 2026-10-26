#include "dispatcher.hpp"
#include "queue.hpp"
#include "close_queue.hpp"
#include "wrapped_message.hpp"

namespace messaging
{
	void dispatcher::wait_and_dispatch()
	{
		for (;;)
		{
			auto msg = q->wait_and_pop();
			dispatch(msg);
		}
	}
	bool dispatcher::dispatch(std::shared_ptr<message_base> const& msg)
	{
		if (dynamic_cast<wrapped_message<close_queue>*>(msg.get()))
		{
			throw close_queue();
		}
		return false;
	}
}