#include "core/actors/ui_actor.hpp"
#include "core/messages/ui_messages.hpp"
#include "libs/messaging/close_queue.hpp"
#include <utility>

interface_machine::interface_machine(atm_core::display_sink sink)
	: sink_(std::move(sink))
{ }

void interface_machine::done()
{
	get_sender().send(messaging::close_queue());
}

void interface_machine::run()
{
	try
	{
		for (;;)
		{
			incoming.wait()
				.handle<issue_money>([&](issue_money const& msg) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("正在出钞 " + std::to_string(msg.amount));
				})
				.handle<display_insufficient_funds>([&](display_insufficient_funds const&) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("余额不足");
				})
				.handle<display_withdrawal_retry>([&](display_withdrawal_retry const&) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("请重新输入取款金额");
				})
				.handle<display_enter_pin>([&](display_enter_pin const&) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("请输入密码（0-9）");
				})
				.handle<display_enter_card>([&](display_enter_card const&) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("请插入银行卡（I）");
				})
				.handle<display_balance>([&](display_balance const& msg) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("您的账户余额为 " + std::to_string(msg.amount));
				})
				.handle<display_withdrawal_options>([&](display_withdrawal_options const&) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("请选择操作：W 取款，B 查余额，C 取消，Q 退卡");
				})
				.handle<display_admin_mode>([&](display_admin_mode const&) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("管理员已登录（仅可查询余额）");
				})
				.handle<display_withdrawal_cancelled>([&](display_withdrawal_cancelled const&) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("取款已取消");
				})
				.handle<display_pin_incorrect_message>([&](display_pin_incorrect_message const& msg) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("密码错误，剩余尝试次数：" + std::to_string(msg.remaining));
				})
				.handle<display_too_many_pin_attempts>([&](display_too_many_pin_attempts const&) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("密码连续错误 3 次，系统将自动退卡");
				})
				.handle<display_pin_ready>([&](display_pin_ready const&) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("密码已输入，请按 Enter 确认");
				})
				.handle<display_enter_withdraw_amount>([&](display_enter_withdraw_amount const&) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("正在取款，请输入金额后按 Enter 或再次点击取款");
				})
				.handle<display_admin_result>([&](display_admin_result const& msg) {
					std::lock_guard<std::mutex> lk(iom);
					sink_(msg.message);
				})
				.handle<eject_card>([&](eject_card const&) {
					std::lock_guard<std::mutex> lk(iom);
					sink_("正在退卡");
				});
		}
	}
	catch (messaging::close_queue const&)
	{ }
}
