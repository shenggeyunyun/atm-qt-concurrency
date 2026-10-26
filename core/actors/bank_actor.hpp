#pragma once
#include <string>
#include <utility>
#include "libs/messaging/receiver.hpp"
#include "libs/messaging/sender.hpp"

class bank_machine
{
	messaging::receiver incoming;
	std::string database_path;
public:
	explicit bank_machine(std::string database_path_ = std::string())
		: database_path(std::move(database_path_))
	{ }
	void done();
	void run();
	messaging::sender get_sender()
	{
		return incoming;
	}
};
