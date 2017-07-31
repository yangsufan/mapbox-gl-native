#pragma once

#include <mbgl/actor/mailbox.hpp>
#include <mbgl/actor/message.hpp>
#include <mbgl/actor/scheduler.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/timer.hpp>

#include <memory>

namespace mbgl {
namespace util {

// Uses the run loop to schedule timers directly
class RunLoopScheduledTask: public Scheduler::Scheduled {
public:
    RunLoopScheduledTask(mbgl::Duration, std::weak_ptr<Mailbox>, std::unique_ptr<Message>);
    ~RunLoopScheduledTask() override;

    void cancel() override;
    bool isFinished() override;

private:
    util::Timer timer;
    bool finished = false;
};

} // namespace util
} // namespace mbgl