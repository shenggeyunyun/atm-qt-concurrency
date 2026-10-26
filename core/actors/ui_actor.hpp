#pragma once
#include <functional>
#include <mutex>
#include <string>
#include "libs/messaging/receiver.hpp"
#include "core/display_sink.hpp"

class interface_machine
{
	messaging::receiver incoming;
	std::mutex iom;
	const atm_core::display_sink sink_;
public:
	explicit interface_machine(atm_core::display_sink sink);
	void done();
	void run();
	messaging::sender get_sender()
	{
		return incoming;
	}
};
