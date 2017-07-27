#include <mbgl/util/default_thread_pool.hpp>
#include <mbgl/actor/actor.hpp>
#include <mbgl/actor/actor_ref.hpp>
#include <mbgl/actor/mailbox.hpp>
#include <mbgl/actor/message.hpp>
#include <mbgl/util/platform.hpp>
#include <mbgl/util/string.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/util/timer.hpp>

namespace {

using namespace mbgl;

// Bridges to the dedicated timer thread
class ThreadPoolScheduledTask : public Scheduler::Scheduled {
public:
    // The Impl is scheduled on the Runloop of the timer thread
    // and uses it to start a timer. When the timer endsm it calls
    // the owning ThreadPoolScheduledTask which is scheduled on the
    // Threadpool
    class Impl {
    public:
        Impl(mbgl::Duration timeout, ActorRef<ThreadPoolScheduledTask> task) {
            timer.start(timeout, Duration::zero(), [&]() {
                task.invoke(&ThreadPoolScheduledTask::onTimer);
            });
        }
        void cancel() {
            timer.stop();
        }
    private:
        util::Timer timer;
    };
    
    ThreadPoolScheduledTask(Scheduler& mainScheduler, util::RunLoop& timerLoop, mbgl::Duration timeout,
                            std::weak_ptr<Mailbox> weakMailbox_, std::unique_ptr<Message> message_)
        : mailbox(std::make_shared<Mailbox>(mainScheduler))
        , impl(timerLoop, timeout, ActorRef<ThreadPoolScheduledTask>(*this, mailbox))
        , weakMailbox(std::move(weakMailbox_))
        , message(std::move(message_)) {
    }
    
    ~ThreadPoolScheduledTask() override {
        close();
    };
    
    void cancel() override {
        close();
    }
    
    bool isFinished() override {
        return finished;
    }
    
    void close() {
        impl.invoke(&ThreadPoolScheduledTask::Impl::cancel);
        mailbox->close();
    }
    
    void onTimer() {
        finished = true;
        if (auto destMailbox = weakMailbox.lock()) {
            destMailbox->push(std::move(message));
        }
    }
    
private:
    std::shared_ptr<Mailbox> mailbox;
    Actor<ThreadPoolScheduledTask::Impl> impl;
    std::weak_ptr<Mailbox> weakMailbox;
    std::unique_ptr<Message> message;
    
    bool finished { false };
};

} // namespace

namespace mbgl {
    

ThreadPool::ThreadPool(std::size_t count) {
    std::promise<void> timerThreadRunning;
    
    timerThread = std::thread([&]() {
        platform::setCurrentThreadName("Timer thread");
        platform::makeThreadLowPriority();
        
        util::RunLoop loop(util::RunLoop::Type::New);
        timerLoop = &loop;
        timerThreadRunning.set_value();
        
        loop.run();
    });
    
    timerThreadRunning.get_future().get();
        
    threads.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        threads.emplace_back([this, i]() {
            platform::setCurrentThreadName(std::string{ "Worker " } + util::toString(i + 1));

            while (true) {
                std::unique_lock<std::mutex> lock(mutex);

                cv.wait(lock, [this] {
                    return !queue.empty() || terminate;
                });

                if (terminate) {
                    return;
                }

                auto mailbox = queue.front();
                queue.pop();
                lock.unlock();

                Mailbox::maybeReceive(mailbox);
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        terminate = true;
    }

    // Stop the threadpool threads
    cv.notify_all();

    for (auto& thread : threads) {
        thread.join();
    }
    
    // Stop the timer thread
    timerLoop->stop();
    timerThread.join();
}

void ThreadPool::schedule(std::weak_ptr<Mailbox> mailbox) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(mailbox);
    }

    cv.notify_one();
}
    
    
std::unique_ptr<Scheduler::Scheduled> ThreadPool::schedule(Duration timeout, std::weak_ptr<Mailbox> mailbox, std::unique_ptr<Message> message) {
    return std::make_unique<ThreadPoolScheduledTask>(*this, *timerLoop, timeout, std::move(mailbox), std::move(message));
}

} // namespace mbgl
