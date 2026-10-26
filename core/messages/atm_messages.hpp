#pragma once
#include <string>

struct card_inserted
{
    std::string account;
    explicit card_inserted(std::string const& account_)
        : account(account_)
    { }
};
struct digit_pressed
{
    char digit;
    explicit digit_pressed(char digit_)
        : digit(digit_)
    { }
};
struct clear_last_pressed
{
};
struct pin_confirm_pressed
{
};
struct withdraw_pressed
{
    unsigned amount;
    explicit withdraw_pressed(unsigned amount_)
        : amount(amount_)
    { }
};
struct withdraw_confirm_pressed
{
};
struct eject_pressed
{
};
struct cancel_pressed
{ };
struct balance_pressed
{ };
