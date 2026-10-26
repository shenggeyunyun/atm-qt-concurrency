#include "core/actors/atm_actor.hpp"
#include "core/messages/atm_messages.hpp"
#include "core/messages/bank_messages.hpp"
#include "core/messages/ui_messages.hpp"
#include "libs/messaging/close_queue.hpp"

void atm::process_withdrawal()
{
	incoming.wait()
		.handle<withdraw_ok>(
			[&](withdraw_ok const& msg)
			{
				interface_hardware.send(
					issue_money(withdrawal_amount));
				bank.send(
					withdrawal_processed(account, withdrawal_amount));
				state = &atm::wait_for_action;
			}
		    )
		.handle<withdraw_denied>(
			[&](withdraw_denied const& msg)
			{
				interface_hardware.send(display_insufficient_funds());
				withdrawal_amount = 0;
				interface_hardware.send(display_withdrawal_retry());
				state = &atm::entering_withdrawal_amount;
			}
		    )
		;
}
void atm::entering_withdrawal_amount()
{
	interface_hardware.send(display_enter_withdraw_amount());
	incoming.wait()
		.handle<withdraw_pressed>([&](withdraw_pressed const& msg) {
			if (msg.amount == 0) return;
			withdrawal_amount = msg.amount;
			bank.send(withdraw(account, msg.amount, incoming));
			state = &atm::process_withdrawal;
		})
		.handle<withdraw_confirm_pressed>([&](withdraw_confirm_pressed const&) {
			if (withdrawal_amount == 0) return;
			bank.send(withdraw(account, withdrawal_amount, incoming));
			state = &atm::process_withdrawal;
		})
		.handle<cancel_pressed>([&](cancel_pressed const&) {
			withdrawal_amount = 0;
			interface_hardware.send(display_withdrawal_cancelled());
			state = &atm::wait_for_action;
		});
}
void atm::process_balance()
{
	incoming.wait()
		.handle<balance>(
			[&](balance const& msg)
			{
				interface_hardware.send(display_balance(msg.amount));
				state = &atm::wait_for_action_after_balance;
			}
		)
		.handle<cancel_pressed>(
			[&](cancel_pressed const& msg)
			{
				state = &atm::done_processing;
			}
		    )
		.handle<eject_pressed>([&](eject_pressed const&) { state = &atm::done_processing; });

}

void atm::wait_for_action_after_balance()
{
	// Keep the balance on the ATM display until the customer starts another action.
	incoming.wait()
		.handle<withdraw_pressed>([&](withdraw_pressed const& msg) {
			if (msg.amount == 0) {
				state = &atm::entering_withdrawal_amount;
			} else {
				withdrawal_amount = msg.amount;
				bank.send(withdraw(account, msg.amount, incoming));
				state = &atm::process_withdrawal;
			}
		})
		.handle<balance_pressed>([&](balance_pressed const&) {
			bank.send(get_balance(account, incoming));
			state = &atm::process_balance;
		})
		.handle<cancel_pressed>([&](cancel_pressed const&) { state = &atm::done_processing; })
		.handle<eject_pressed>([&](eject_pressed const&) { state = &atm::done_processing; })
		.handle<admin_operation_result>([&](admin_operation_result const& msg) {
			interface_hardware.send(display_admin_result(msg.success, msg.message));
		})
		.handle<admin_create_account>([&](admin_create_account const& msg) {
			if (admin_session) bank.send(admin_create_account(account, msg.account, msg.pin, msg.initial_balance, incoming));
		})
		.handle<admin_set_balance>([&](admin_set_balance const& msg) {
			if (admin_session) bank.send(admin_set_balance(account, msg.account, msg.amount, incoming));
		})
		.handle<admin_delete_account>([&](admin_delete_account const& msg) {
			if (admin_session) bank.send(admin_delete_account(account, msg.account, incoming));
		});
}
void atm::wait_for_action() 
{
	interface_hardware.send(display_withdrawal_options());
	incoming.wait()
		.handle<withdraw_pressed>(
			[&](withdraw_pressed const& msg)
			{
				if (msg.amount == 0) {
					state = &atm::entering_withdrawal_amount;
				} else {
					withdrawal_amount = msg.amount;
					bank.send(withdraw(account, msg.amount, incoming));
					state = &atm::process_withdrawal;
				}
			}
		    )
		.handle<balance_pressed>(
			[&](balance_pressed const& msg)
			{
				bank.send(get_balance(account, incoming));
				state = &atm::process_balance;
			}
		    )
		.handle<cancel_pressed>(
			[&](cancel_pressed const& msg)
			{
				state = &atm::done_processing;
			}
		    )
		.handle<eject_pressed>([&](eject_pressed const&) { state = &atm::done_processing; })
		.handle<admin_operation_result>([&](admin_operation_result const& msg) {
			interface_hardware.send(display_admin_result(msg.success, msg.message));
		})
		.handle<admin_create_account>([&](admin_create_account const& msg) {
			if (admin_session) bank.send(admin_create_account(account, msg.account, msg.pin, msg.initial_balance, incoming));
		})
		.handle<admin_set_balance>([&](admin_set_balance const& msg) {
			if (admin_session) bank.send(admin_set_balance(account, msg.account, msg.amount, incoming));
		})
		.handle<admin_delete_account>([&](admin_delete_account const& msg) {
			if (admin_session) bank.send(admin_delete_account(account, msg.account, incoming));
		});
}
void atm::verifying_pin() 
{
	incoming.wait()
		.handle<pin_verified>(
			[&](pin_verified const& msg)
			{
				admin_session = msg.is_admin;
				if (msg.is_admin) {
					interface_hardware.send(display_admin_mode());
					state = &atm::done_processing;
					return;
				}
				state = &atm::wait_for_action;
			}
		)
		.handle<pin_incorrect>(
			[&](pin_incorrect const& msg)
			{
				++pin_attempts;
				interface_hardware.send(display_pin_incorrect_message(3 - pin_attempts));
				if (pin_attempts >= 3) {
					interface_hardware.send(display_too_many_pin_attempts());
					state = &atm::done_processing;
				} else {
					pin.clear();
					interface_hardware.send(display_enter_pin());
					state = &atm::getting_pin;
				}
			}
		)
		.handle<cancel_pressed>(
			[&](cancel_pressed const& msg)
			{
				state = &atm::done_processing;
			}
		);
}
void atm::getting_pin()
{
	incoming.wait()
		.handle<digit_pressed>(
			[&](digit_pressed const& msg)
			{
				unsigned const pin_length = 4;
				if (pin.length() >= pin_length || msg.digit < '0' || msg.digit > '9') return;
				pin += msg.digit;
				if (pin.length() == pin_length)
					interface_hardware.send(display_pin_ready());
			}
		)
		.handle<clear_last_pressed>(
			[&](clear_last_pressed const& msg)
			{
				if (!pin.empty())
				{
					pin.pop_back();
				}
			}
		)
		.handle<pin_confirm_pressed>(
			[&](pin_confirm_pressed const&)
			{
				if (pin.length() == 4) {
					bank.send(verify_pin(account, pin, incoming));
					state = &atm::verifying_pin;
				}
			}
		)
		.handle <cancel_pressed>(
			[&](cancel_pressed const& msg)
			{
				state = &atm::done_processing;
			}
		);
}
void atm::waiting_for_card()
{
	interface_hardware.send(display_enter_card());
	incoming.wait()
		.handle<card_inserted>(
			[&](card_inserted const& msg)
			{
				account = msg.account;
				pin = "";
				pin_attempts = 0;
				interface_hardware.send(display_enter_pin());
				state = &atm::getting_pin;
			}
		);
}
void atm::done_processing()
{
	interface_hardware.send(eject_card());
	state = &atm::waiting_for_card;
}
void atm::done()
{
	get_sender().send(messaging::close_queue());
}
void atm::run()
{
	state = &atm::waiting_for_card;
	try
	{
		for (;;)
		{
			(this->*state)();
		}
	}
	catch (messaging::close_queue const&)
	{ }
}
