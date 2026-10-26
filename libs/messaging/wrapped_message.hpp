#pragma once
#include "message_base.hpp"

namespace messaging
{
	template<typename Msg>
	struct wrapped_message :
		message_base
	{
		Msg contents;
		explicit wrapped_message(Msg const& contents_) :
			contents(contents_)
		{ }
	};
}