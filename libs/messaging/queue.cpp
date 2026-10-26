#include "queue.hpp"

namespace messaging
{
    std::shared_ptr<message_base> queue::wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(m);
        c.wait(lk, [this] { return !q.empty(); });
        std::shared_ptr<message_base> res = std::move(q.front());
        q.pop();
        return res;
    }
}