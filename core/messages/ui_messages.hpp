 #pragma once
#include <string>
#include <utility>

struct display_enter_pin
{ };
struct display_enter_card
{ };
struct display_insufficient_funds
{ };
struct display_withdrawal_retry
{ };
struct display_withdrawal_cancelled
{ };
struct display_pin_incorrect_message
{
    unsigned remaining;
    explicit display_pin_incorrect_message(unsigned remaining_ = 0) : remaining(remaining_) { }
};
struct display_too_many_pin_attempts
{ };
struct display_pin_ready
{ };
struct display_enter_withdraw_amount
{ };
struct display_admin_result
{
    bool success;
    std::string message;
    display_admin_result(bool ok, std::string text) : success(ok), message(std::move(text)) { }
};
struct display_withdrawal_options
{ };
struct display_admin_mode
{ };
struct issue_money
{
    unsigned amount;
    issue_money(unsigned amount_)
        : amount(amount_)
    { }
};
struct eject_card
{ };
struct display_balance
{
    unsigned amount;
    explicit display_balance(unsigned amount_)
        : amount(amount_)
    { }
};
