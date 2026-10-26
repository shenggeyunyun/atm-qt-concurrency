#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "core/actors/ui_actor.hpp"
#include "core/messages/ui_messages.hpp"
#include "libs/messaging/close_queue.hpp"
#include "libs/messaging/queue.hpp"
#include "libs/messaging/receiver.hpp"
#include "libs/messaging/sender.hpp"
#include "libs/messaging/wrapped_message.hpp"

namespace {
struct A { int value; explicit A(int v = 0) : value(v) {} };
struct B { int value; explicit B(int v = 0) : value(v) {} };
struct C { int value; explicit C(int v = 0) : value(v) {} };

template<typename T>
T const& unwrap(std::shared_ptr<messaging::message_base> const& msg)
{
    return dynamic_cast<messaging::wrapped_message<T> const&>(*msg).contents;
}
}

TEST(Queue, PushThenPopPreservesTypeAndValue)
{
    messaging::queue q;
    q.push(A(42));
    EXPECT_EQ(unwrap<A>(q.wait_and_pop()).value, 42);
}

TEST(Queue, EmptyPopBlocksUntilPush)
{
    messaging::queue q;
    std::atomic<bool> returned(false);
    std::thread t([&] { q.wait_and_pop(); returned = true; });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(returned.load());
    q.push(A(7));
    t.join();
    EXPECT_TRUE(returned.load());
}

TEST(Queue, ConcurrentPushAndConsumeAllMessages)
{
    messaging::queue q;
    std::atomic<int> produced(0);
    std::atomic<int> consumed(0);
    std::thread p1([&] { for (int i = 0; i < 500; ++i) { q.push(A(i)); ++produced; } });
    std::thread p2([&] { for (int i = 0; i < 500; ++i) { q.push(A(i)); ++produced; } });
    std::thread c([&] { for (int i = 0; i < 1000; ++i) { q.wait_and_pop(); ++consumed; } });
    p1.join(); p2.join(); c.join();
    EXPECT_EQ(produced.load(), 1000);
    EXPECT_EQ(consumed.load(), 1000);
}

TEST(Sender, DefaultConstructedSendIsNoOp)
{
    messaging::sender s;
    EXPECT_NO_THROW(s.send(A(1)));
}

TEST(Dispatcher, CloseQueueDestructorThrows)
{
    messaging::receiver r;
    messaging::sender s = r;
    s.send(messaging::close_queue());
    EXPECT_THROW(r.wait(), messaging::close_queue);
}

TEST(Dispatcher, HandleAInvokesOnceThenCloses)
{
    messaging::receiver r;
    messaging::sender s = r;
    int calls = 0;
    std::thread t([&] {
        try { r.wait().handle<A>([&](A const&) { ++calls; }); }
        catch (messaging::close_queue const&) {}
    });
    s.send(A(1)); s.send(messaging::close_queue()); t.join();
    EXPECT_EQ(calls, 1);
}

TEST(Dispatcher, UnmatchedBFallsBackToPreviousDispatcher)
{
    messaging::receiver r;
    messaging::sender s = r;
    int calls = 0;
    std::thread t([&] {
        try { r.wait().handle<A>([&](A const&) { ++calls; }); }
        catch (messaging::close_queue const&) {}
    });
    s.send(B(1)); s.send(messaging::close_queue()); t.join();
    EXPECT_EQ(calls, 0);
}

TEST(Dispatcher, ChainedHandlersDispatchEachType)
{
    int a = 0, b = 0, c = 0;
    messaging::receiver ra, rb, rc;
    std::thread ta([&] { try { ra.wait().handle<A>([&](A const&) { ++a; }).handle<B>([](B const&) {}).handle<C>([](C const&) {}); } catch (messaging::close_queue const&) {} });
    std::thread tb([&] { try { rb.wait().handle<A>([](A const&) {}).handle<B>([&](B const&) { ++b; }).handle<C>([](C const&) {}); } catch (messaging::close_queue const&) {} });
    std::thread tc([&] { try { rc.wait().handle<A>([](A const&) {}).handle<B>([](B const&) {}).handle<C>([&](C const&) { ++c; }); } catch (messaging::close_queue const&) {} });
    messaging::sender(ra).send(A()); messaging::sender(ra).send(messaging::close_queue());
    messaging::sender(rb).send(B()); messaging::sender(rb).send(messaging::close_queue());
    messaging::sender(rc).send(C()); messaging::sender(rc).send(messaging::close_queue());
    ta.join(); tb.join(); tc.join();
    EXPECT_EQ(a, 1); EXPECT_EQ(b, 1); EXPECT_EQ(c, 1);
}

TEST(Dispatcher, ChainedHandlersDoNotDoubleDispatch)
{
    messaging::receiver r;
    messaging::sender s = r;
    int calls = 0;
    std::thread t([&] {
        try {
            r.wait().handle<A>([&](A const&) { ++calls; })
                .handle<B>([&](B const&) { ++calls; });
        } catch (messaging::close_queue const&) {}
    });
    s.send(A()); s.send(messaging::close_queue()); t.join();
    EXPECT_EQ(calls, 1);
}

TEST(SenderReceiver, ImplicitSenderReturnsToOwnQueue)
{
    messaging::receiver r;
    messaging::sender s = r;
    int value = 0;
    std::thread t([&] {
        try { r.wait().handle<A>([&](A const& msg) { value = msg.value; }); }
        catch (messaging::close_queue const&) {}
    });
    s.send(A(9));
    s.send(messaging::close_queue());
    t.join();
    EXPECT_EQ(value, 9);
}

TEST(SenderReceiver, MultipleSendersShareQueue)
{
    messaging::receiver r;
    messaging::sender first = r;
    messaging::sender second = r;
    std::vector<int> values;
    std::mutex values_mutex;
    std::thread t1([&] { try { r.wait().handle<A>([&](A const& msg) { std::lock_guard<std::mutex> lock(values_mutex); values.push_back(msg.value); }); } catch (messaging::close_queue const&) {} });
    std::thread t2([&] { try { r.wait().handle<A>([&](A const& msg) { std::lock_guard<std::mutex> lock(values_mutex); values.push_back(msg.value); }); } catch (messaging::close_queue const&) {} });
    first.send(A(1)); second.send(A(2));
    first.send(messaging::close_queue()); second.send(messaging::close_queue());
    t1.join(); t2.join();
    ASSERT_EQ(values.size(), 2u);
    EXPECT_TRUE((values[0] == 1 && values[1] == 2) || (values[0] == 2 && values[1] == 1));
}

TEST(InterfaceMachine, SinkReceivesExactDisplayLines)
{
    std::vector<std::string> lines;
    std::mutex lines_mutex;
    interface_machine machine([&](std::string const& line) {
        std::lock_guard<std::mutex> lock(lines_mutex);
        lines.push_back(line);
    });
    messaging::sender s = machine.get_sender();
    std::thread t(&interface_machine::run, &machine);
    s.send(issue_money(50));
    s.send(display_insufficient_funds());
    s.send(display_enter_pin());
    s.send(display_enter_card());
    s.send(display_balance(149));
    s.send(display_withdrawal_options());
    s.send(display_withdrawal_cancelled());
    s.send(display_pin_incorrect_message());
    s.send(eject_card());
    machine.done();
    t.join();
    ASSERT_EQ(lines.size(), 11u);
    EXPECT_EQ(lines[0], "正在出钞 50");
    EXPECT_EQ(lines[1], "余额不足");
    EXPECT_EQ(lines[2], "请输入密码（0-9）");
    EXPECT_EQ(lines[3], "请插入银行卡（I）");
    EXPECT_EQ(lines[4], "您的账户余额为 149");
    EXPECT_EQ(lines[5], "请输入取款金额后确认");
    EXPECT_EQ(lines[6], "查询余额？（b）");
    EXPECT_EQ(lines[7], "取消交易？（c）");
    EXPECT_EQ(lines[8], "取款已取消");
    EXPECT_EQ(lines[9], "密码错误");
    EXPECT_EQ(lines[10], "正在退卡");
}
