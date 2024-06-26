#include "sung/general/time.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>


namespace {

    // Duration cast
    double dur_cast(std::chrono::steady_clock::duration dur) {
        namespace chr = std::chrono;
        return chr::duration_cast<chr::duration<double>>(dur).count();
    }

}  // namespace


namespace sung {

    void sleep_naive(double seconds) {
        std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
    }

    void sleep_loop(double seconds) {
        using clock_t = std::chrono::steady_clock;
        using duration_t = std::chrono::duration<double>;
        constexpr static duration_t MIN_SLEEP_DURATION(0);

        const auto start = clock_t::now();
        while (duration_t(clock_t::now() - start).count() < seconds) {
            std::this_thread::sleep_for(MIN_SLEEP_DURATION);
        }
    }

    void sleep_hybrid(double seconds, double proportion) {
        using clock_t = std::chrono::steady_clock;
        using duration_t = std::chrono::duration<double>;
        constexpr static duration_t MIN_SLEEP_DURATION(0);

        const auto start = clock_t::now();
        std::this_thread::sleep_for(duration_t(seconds * proportion));
        while (duration_t(clock_t::now() - start).count() < seconds) {
            std::this_thread::sleep_for(MIN_SLEEP_DURATION);
        }
    }

}  // namespace sung


// CalenderTime
namespace sung {

    bool CalenderTime::operator==(const CalenderTime& rhs) const {
        return value_ == rhs.value_;
    }

    bool CalenderTime::operator!=(const CalenderTime& rhs) const {
        return value_ != rhs.value_;
    }

    CalenderTime CalenderTime::from_now() {
        return CalenderTime{ Clock_t::now() };
    }

    CalenderTime CalenderTime::from_total_sec(double total_seconds) {
        namespace chr = std::chrono;
        const auto duration = chr::duration<double>(total_seconds);
        const auto ns = chr::duration_cast<Clock_t::duration>(duration);
        return CalenderTime{ Clock_t::time_point{ ns } };
    }

    CalenderTime CalenderTime::from_time_point(Clock_t::time_point time_point) {
        return CalenderTime{ time_point };
    }

    CalenderTime CalenderTime::from_time_t(time_t time) {
        return CalenderTime{ Clock_t::from_time_t(time) };
    }

    double CalenderTime::to_total_seconds() const {
        namespace chr = std::chrono;
        const auto since = Clock_t::now().time_since_epoch();
        const auto nanoseconds = chr::duration_cast<chr::nanoseconds>(since);
        return nanoseconds.count() / 1e9;
    }

    CalenderTime::Clock_t::time_point CalenderTime::to_time_point() const {
        return value_;
    }

    time_t CalenderTime::to_time_t() const {
        return std::chrono::system_clock::to_time_t(value_);
    }

    std::string CalenderTime::make_locale_text() const {
        const auto time = this->to_time_t();
        auto output = std::string{ std::ctime(&time) };
        if (output.back() == '\n')
            output.pop_back();

        return output;
    }

    std::string CalenderTime::make_sortable_text(bool utc) const {
        const auto time = this->to_time_t();
        const auto tm = *(utc ? gmtime(&time) : localtime(&time));
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
        return ss.str();
    }

    CalenderTime::HumanReadable CalenderTime::local_time() const {
        const auto time = this->to_time_t();
        const auto tm = *localtime(&time);
        return HumanReadable{ tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                              tm.tm_hour,        tm.tm_min,     tm.tm_sec };
    }

    CalenderTime::HumanReadable CalenderTime::utc_time() const {
        const auto time = this->to_time_t();
        const auto tm = *gmtime(&time);
        return HumanReadable{ tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                              tm.tm_hour,        tm.tm_min,     tm.tm_sec };
    }

}  // namespace sung


// MonotonicClock
namespace sung {

    double MonotonicClock::elapsed() const {
        return ::dur_cast(Clock_t::now() - last_checked_);
    }

    void MonotonicClock::set_min() { last_checked_ = Clock_t::time_point{}; }

    void MonotonicClock::check() { last_checked_ = Clock_t::now(); }

    double MonotonicClock::check_get_elapsed() {
        const auto now = Clock_t::now();
        const auto elapsed = ::dur_cast(now - last_checked_);
        last_checked_ = now;
        return elapsed;
    }

    bool MonotonicClock::check_if_elapsed(double seconds) {
        const auto now = Clock_t::now();
        const auto elapsed = ::dur_cast(now - last_checked_);
        if (elapsed >= seconds) {
            last_checked_ = now;
            return true;
        }
        return false;
    }

}  // namespace sung


// ManualClock
namespace sung {

    double ManualClock::elapsed() const { return cur_time_ - last_checked_; }

    void ManualClock::set_min() { this->set(0); }

    void ManualClock::check() { last_checked_ = cur_time_; }

    double ManualClock::check_get_elapsed() {
        const auto result = this->elapsed();
        this->check();
        return result;
    }

    bool ManualClock::check_if_elapsed(double seconds) {
        if (this->elapsed() >= seconds) {
            this->check();
            return true;
        }
        return false;
    }

    void ManualClock::add(double value) {
        cur_time_ += value;
    }

    void ManualClock::set(double value) {
        cur_time_ = value;
    }

    void ManualClock::set_max() {
        this->set(999999999999999);  // Such random number
    }

}  // namespace sung
