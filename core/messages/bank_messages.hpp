#pragma once
#include <string>
#include "libs/messaging/sender.hpp"

struct withdraw
{
    std::string account;
    unsigned amount;
    mutable messaging::sender atm_queue;
    withdraw(std::string const& account_,
        unsigned amount_,
        messaging::sender atm_queue_)
        : account(account_), amount(amount_), atm_queue(atm_queue_)
    { }
};
struct withdraw_ok
{ };
struct withdraw_denied
{ };
struct cancel_withdrawal
{
    std::string account;
    unsigned amount;
    cancel_withdrawal(std::string const& account_, unsigned amount_)
        : account(account_), amount(amount_)
    { }
};
struct withdrawal_processed
{
    std::string account;
    unsigned amount;
    withdrawal_processed(std::string const& account_, unsigned amount_)
        : account(account_), amount(amount_)
    { }
};
struct verify_pin
{
    std::string account;
    std::string pin;
    mutable messaging::sender atm_queue;
    verify_pin(std::string const& account_,
        std::string const& pin_,
        messaging::sender atm_queue_)
        : account(account_), pin(pin_), atm_queue(atm_queue_)
    { }
};
struct pin_verified
{
    bool is_admin;
    explicit pin_verified(bool admin = false) : is_admin(admin) { }
};
struct pin_incorrect
{ };
struct get_balance
{
    std::string account;
    mutable messaging::sender atm_queue;
    get_balance(std::string const& account_, messaging::sender atm_queue_)
        : account(account_), atm_queue(atm_queue_)
    { }
};
struct balance
{
    unsigned amount;
    explicit balance(unsigned amount_)
        : amount(amount_)
    { }
};
struct admin_create_account
{
    std::string admin_account, account, pin;
    unsigned initial_balance;
    mutable messaging::sender atm_queue;
    admin_create_account(std::string const& admin, std::string const& target, std::string const& pin_, unsigned value, messaging::sender queue)
        : admin_account(admin), account(target), pin(pin_), initial_balance(value), atm_queue(queue) { }
};
struct admin_set_balance
{
    std::string admin_account, account;
    unsigned amount;
    mutable messaging::sender atm_queue;
    admin_set_balance(std::string const& admin, std::string const& target, unsigned value, messaging::sender queue)
        : admin_account(admin), account(target), amount(value), atm_queue(queue) { }
};
struct admin_delete_account
{
    std::string admin_account, account;
    mutable messaging::sender atm_queue;
    admin_delete_account(std::string const& admin, std::string const& target, messaging::sender queue)
        : admin_account(admin), account(target), atm_queue(queue) { }
};
struct admin_operation_result
{
    bool success;
    std::string message;
    explicit admin_operation_result(bool ok = false, std::string text = std::string()) : success(ok), message(std::move(text)) { }
};
