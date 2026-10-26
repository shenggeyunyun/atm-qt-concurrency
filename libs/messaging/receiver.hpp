#pragma once
#include "queue.hpp"
#include "sender.hpp"
#include "dispatcher.hpp"

namespace messaging
{
	class receiver
	{
		queue q;
	public:
		operator sender()
		{
			return sender(&q);
		}
		dispatcher wait()
		{
			return dispatcher(&q);
		}
	};
}