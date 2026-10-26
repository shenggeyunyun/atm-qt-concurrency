#include "core/actors/bank_actor.hpp"
#include "core/messages/bank_messages.hpp"
#include "core/messages/atm_messages.hpp"
#include "core/bank_database.hpp"
#include "libs/messaging/close_queue.hpp"

void bank_machine::done()
{
	get_sender().send(messaging::close_queue());
}
void bank_machine::run()
{
	try
	{
		bank_database database(database_path);
		for (;;)
		{
			incoming.wait()
				.handle<verify_pin>(
					[&](verify_pin const& msg)
					{
						bank_account_record account = database.authenticate(msg.account, msg.pin);
						if (account.exists && account.pin_matches)
						{
							msg.atm_queue.send(pin_verified(account.is_admin));
						}
						else
						{
							msg.atm_queue.send(pin_incorrect());
						}
					}
				    )
				.handle<withdraw>(
					[&](withdraw const& msg)
					{
						if (database.withdraw(msg.account, msg.amount))
						{
							msg.atm_queue.send(withdraw_ok());
						}
						else
						{
							msg.atm_queue.send(withdraw_denied());
						}
					}
				    )
				.handle<get_balance>(
					[&](get_balance const& msg)
					{
						msg.atm_queue.send(::balance(database.balance(msg.account)));
					}
				    )
				.handle<withdrawal_processed>(
					[&](withdrawal_processed const& msg)
					{ }
				    )
				.handle<cancel_withdrawal>(
					[&](cancel_withdrawal const& msg)
					{ }
				)
				.handle<admin_create_account>([&](admin_create_account const& msg) {
					bool ok = database.is_admin_account(msg.admin_account) && database.create_account(msg.account, msg.pin, msg.initial_balance);
					msg.atm_queue.send(admin_operation_result(ok, ok ? "账户创建成功" : "账户创建失败（账号可能已存在）"));
				})
				.handle<admin_set_balance>([&](admin_set_balance const& msg) {
					bool ok = database.is_admin_account(msg.admin_account) && database.set_balance(msg.account, msg.amount);
					msg.atm_queue.send(admin_operation_result(ok, ok ? "余额修改成功" : "余额修改失败（账号不存在或为管理员）"));
				})
				.handle<admin_delete_account>([&](admin_delete_account const& msg) {
					bool ok = database.is_admin_account(msg.admin_account) && database.delete_account(msg.account);
					msg.atm_queue.send(admin_operation_result(ok, ok ? "账户已删除" : "删除失败（账号不存在或为管理员）"));
				});
		}
	}
	catch(messaging::close_queue const&)
	{ }
}
