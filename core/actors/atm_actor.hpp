#pragma once
#include <string>
#include "libs/messaging/receiver.hpp"
#include "libs/messaging/sender.hpp"

class atm
{
	messaging::receiver incoming;
	messaging::sender bank;
	messaging::sender interface_hardware;
	void (atm::* state)() = nullptr;
	std::string account;
	unsigned withdrawal_amount = 0;
	std::string pin;
	unsigned pin_attempts = 0;
	bool admin_session = false;
	void process_withdrawal();
	void entering_withdrawal_amount();
	void process_balance();
	void wait_for_action();
	void wait_for_action_after_balance();
	void verifying_pin();
	void getting_pin();
	void waiting_for_card();
	void done_processing();
	atm(atm const&) = delete;
	atm& operator=(atm const&) = delete;
public:
	atm(messaging::sender bank_,
		messaging::sender interface_hardware_):
		bank(bank_),interface_hardware(interface_hardware_)
	{ }
	void done();
    void run();
	messaging::sender get_sender() 
	{
		return incoming; 
	}
};
