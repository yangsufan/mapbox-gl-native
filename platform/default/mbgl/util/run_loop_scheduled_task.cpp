#include <mbgl/util/run_loop_scheduled_task.hpp>

#include <functional>

namespace  {

using namespace mbgl;

// Wraps a functor in a movable/copyable type
// by moving it to the heap in a shared_ptr
template <class Fn>
class FunctorWrapper
{
public:
    FunctorWrapper(Fn fn_) : fn(std::make_shared<Fn>(std::move(fn_))) {};
    void operator()(){
        (*fn)();
    }
private:
    std::shared_ptr<Fn> fn;
};

// Wraps a Functor in a movable/copyable type
// so it adheres to the std::function requirements
template <class Fn>
std::function<void ()> wrap(Fn&& fn) {
    return { FunctorWrapper<Fn> { std::move(fn) } };
}

} // namespace

namespace mbgl {
namespace util {

RunLoopScheduledTask::RunLoopScheduledTask(mbgl::Duration timeout, std::weak_ptr<Mailbox> weakMailbox, std::unique_ptr<Message> message) {
    timer.start(timeout,
        mbgl::Duration::zero(),
        wrap(
        [&, weakMailbox = std::move(weakMailbox), message = std::move(message)] () mutable {
            if (auto mailbox = weakMailbox.lock()) {
                mailbox->push(std::forward<std::unique_ptr<Message>>(message));
            }
            finished = true;
        })
    );
}

RunLoopScheduledTask::~RunLoopScheduledTask() = default;

void RunLoopScheduledTask::cancel() {
    timer.stop();
}

bool RunLoopScheduledTask::isFinished() {
    return finished;
}

} // namespace util
} // namespace mbgl
