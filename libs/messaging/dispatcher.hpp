#pragma once
#include <memory>
#include <utility>
#include "template_dispatcher.hpp"
#include "message_base.hpp"

namespace messaging
{
	class queue;
	class close_queue;
	class dispatcher
	{
		queue* q;
		bool chained;
		dispatcher(dispatcher const&) = delete;
		dispatcher& operator=(dispatcher const&) = delete;
		template<typename Dispatcher, typename Msg, typename Func>
		friend class TemplateDispatcher;
		void wait_and_dispatch();
		bool dispatch(std::shared_ptr<message_base> const& msg);
	public:
		dispatcher(dispatcher&& other) noexcept
			: q(other.q), chained(other.chained)
		{
			other.chained = true;
		}
		explicit dispatcher(queue* q_)
			: q(q_), chained(false)
		{ }
		template<typename Message, typename Func>
		TemplateDispatcher<dispatcher, Message, Func>
			handle(Func&& f)
		{
			return TemplateDispatcher<dispatcher, Message, Func>(
				q, this, std::forward<Func>(f));
		}
		~dispatcher() noexcept(false)
		{
			if (!chained) 
			{ 
				wait_and_dispatch(); 
			}
		}
	};
}