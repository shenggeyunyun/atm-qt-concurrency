#pragma once
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include "message_base.hpp"
#include "wrapped_message.hpp"

namespace messaging
{
	class queue
	{
		std::mutex m;
		std::condition_variable c;
		std::queue<std::shared_ptr<message_base>> q;
	public:
		template<typename T>
		void push(T const& msg)
		{
			std::lock_guard<std::mutex> lk(m);
			q.push(std::make_shared<wrapped_message<T>> (msg));
			c.notify_all();
		}
		std::shared_ptr<message_base> wait_and_pop();
	};
}