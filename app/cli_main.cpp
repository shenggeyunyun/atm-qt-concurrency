#ifndef ATM_QT_APP
#include <thread>
#include <iostream>
#include <limits>
#include <string>
#include <cctype>
#include <condition_variable>
#include <mutex>
#include <QCoreApplication>
#include "core/actors/atm_actor.hpp"
#include "core/actors/bank_actor.hpp"
#include "core/actors/ui_actor.hpp"
#include "core/messages/atm_messages.hpp"
#include "libs/messaging/close_queue.hpp"
int main(int argc, char** argv)
{
    enum class CliMode { WaitingForCard, AwaitingPin, PinEntry, PinVerificationPending, Action, WithdrawalAmount, OperationPending };
    QCoreApplication app(argc, argv);
    bank_machine bank;
    std::mutex output_mutex;
    std::condition_variable output_ready;
    bool withdrawal_finished = false;
    unsigned pin_digits_entered = 0;
    CliMode mode = CliMode::WaitingForCard;
    interface_machine interface_hardwatre([&](std::string const& text) {
        std::cout << text << std::endl;
        std::lock_guard<std::mutex> lock(output_mutex);
        if (text == "请插入银行卡（I）" || text == "正在退卡") mode = CliMode::WaitingForCard;
        else if (text == "请输入密码（0-9）") {
            mode = CliMode::PinEntry;
            pin_digits_entered = 0;
        } else if (text.find("请选择操作：") == 0) mode = CliMode::Action;
        else if (text.find("您的账户余额为 ") == 0) {
            mode = CliMode::Action;
            withdrawal_finished = true;
            output_ready.notify_one();
        } else if (text == "取款已取消") mode = CliMode::Action;
        else if (text.find("正在出钞 ") == 0) mode = CliMode::Action;
        else if (text == "请重新输入取款金额") mode = CliMode::WithdrawalAmount;
        if (text == "正在取款，请输入金额后按 Enter 或再次点击取款" || text == "余额不足") {
            mode = CliMode::WithdrawalAmount;
        }
        if (text == "余额不足" || text.find("正在出钞 ") == 0) {
            withdrawal_finished = true;
            output_ready.notify_one();
        }
    });
    atm machine(bank.get_sender(), interface_hardwatre.get_sender());
    std::thread bank_thread(&bank_machine::run, &bank);
    std::thread if_thread(&interface_machine::run, &interface_hardwatre);
    std::thread atm_thread(&atm::run, &machine);
    messaging::sender atmqueue(machine.get_sender());
    const auto submit_withdrawal = [&](std::string const& input) {
        unsigned long long parsed = 0;
        bool valid = !input.empty();
        for (char digit : input) {
            if (digit < '0' || digit > '9') { valid = false; break; }
            const unsigned value = static_cast<unsigned>(digit - '0');
            if (parsed > (static_cast<unsigned long long>(std::numeric_limits<int>::max()) - value) / 10) {
                valid = false;
                break;
            }
            parsed = parsed * 10 + value;
        }
        if (!valid || parsed == 0) {
            std::cout << "金额无效，请输入 1 到 2147483647 之间的整数。" << std::endl;
            return;
        }
        {
            std::lock_guard<std::mutex> lock(output_mutex);
            withdrawal_finished = false;
            mode = CliMode::OperationPending;
        }
        atmqueue.send(withdraw_pressed(static_cast<unsigned>(parsed)));
        std::unique_lock<std::mutex> lock(output_mutex);
        output_ready.wait(lock, [&] { return withdrawal_finished; });
    };
    bool quit_pressed = false;
    while (!quit_pressed)
    {
        std::string command;
        if (!(std::cin >> command)) break;
        const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(command[0])));
        if (c >= '0' && c <= '9') {
            CliMode current_mode;
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                current_mode = mode;
            }
            if (current_mode == CliMode::WithdrawalAmount) {
                submit_withdrawal(command);
                continue;
            }
            if (current_mode != CliMode::PinEntry) {
                std::cout << "当前不接受 PIN 输入。" << std::endl;
                continue;
            }
            if (command.size() == 1) {
                atmqueue.send(digit_pressed(c));
                std::lock_guard<std::mutex> lock(output_mutex);
                if (pin_digits_entered < 4) ++pin_digits_entered;
                continue;
            }
            bool pin_token = command.size() == 4;
            for (char digit : command) pin_token = pin_token && digit >= '0' && digit <= '9';
            if (!pin_token || pin_digits_entered != 0) {
                std::cout << "请输入 4 位数字 PIN，或逐位输入。" << std::endl;
                continue;
            }
            for (char digit : command) atmqueue.send(digit_pressed(digit));
            atmqueue.send(pin_confirm_pressed());
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                pin_digits_entered = 0;
                mode = CliMode::PinVerificationPending;
            }
            continue;
        }
        switch (c)
        {
        case 'b':
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                if (mode != CliMode::Action) {
                    std::cout << "请先完成登录。" << std::endl;
                    break;
                }
                withdrawal_finished = false;
                mode = CliMode::OperationPending;
            }
            atmqueue.send(balance_pressed());
            {
                std::unique_lock<std::mutex> lock(output_mutex);
                output_ready.wait(lock, [&] { return withdrawal_finished; });
            }
            break;
        case 'p':
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                if (mode != CliMode::PinEntry || pin_digits_entered != 4) {
                    std::cout << "请输入完整的 4 位 PIN 后再确认。" << std::endl;
                    break;
                }
                pin_digits_entered = 0;
                mode = CliMode::PinVerificationPending;
            }
            atmqueue.send(pin_confirm_pressed());
            break;
        case 'c':
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                if (mode != CliMode::PinEntry && mode != CliMode::WithdrawalAmount && mode != CliMode::Action) {
                    std::cout << "当前操作尚未完成，暂不能取消。" << std::endl;
                    break;
                }
                mode = CliMode::WaitingForCard;
            }
            atmqueue.send(cancel_pressed());
            break;
        case 'q':
            quit_pressed = true;
            break;
        case 'i':
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                if (mode != CliMode::WaitingForCard) {
                    std::cout << "请先退出现有会话。" << std::endl;
                    break;
                }
                mode = CliMode::AwaitingPin;
            }
            std::cout << "银行卡号: ";
            {
                std::string account;
                if (!(std::cin >> account)) {
                    quit_pressed = true;
                    break;
                }
                atmqueue.send(card_inserted(account));
            }
            break;
        case 'w':
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                if (mode != CliMode::Action && mode != CliMode::WithdrawalAmount) {
                    std::cout << "请先完成登录并进入操作菜单。" << std::endl;
                    break;
                }
            }
            std::cout << "取款金额: ";
            {
                std::string input = command.size() > 1 ? command.substr(1) : std::string();
                if (input.empty() && !(std::cin >> input)) {
                    quit_pressed = true;
                    break;
                }
                submit_withdrawal(input);
            }
            break;
        }
    }
    machine.done();
    atm_thread.join();
    bank.done();
    bank_thread.join();
    interface_hardwatre.done();
    if_thread.join();
}
#endif
