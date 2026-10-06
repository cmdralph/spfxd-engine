#include <coffee/clock.h>

#include <algorithm>

namespace coffee {

clock::clock() noexcept : start_(underlying_clock::now()) {}

double clock::elapsed_seconds() const noexcept {
    return std::chrono::duration<double>(underlying_clock::now() - start_).count();
}

double clock::restart() noexcept {
    const auto now = underlying_clock::now();
    const double elapsed = std::chrono::duration<double>(now - start_).count();
    start_ = now;
    return elapsed;
}

frame_timer::frame_timer(double maximum_delta_seconds) noexcept
    : maximum_delta_(std::max(0.0, maximum_delta_seconds)) {}

double frame_timer::tick() noexcept {
    delta_ = std::min(clock_.restart(), maximum_delta_);
    elapsed_ += delta_;
    smoothed_delta_ = smoothed_delta_ == 0.0
        ? delta_
        : smoothed_delta_ * 0.9 + delta_ * 0.1;
    return delta_;
}

double frame_timer::delta_seconds() const noexcept { return delta_; }
double frame_timer::elapsed_seconds() const noexcept { return elapsed_; }
double frame_timer::frames_per_second() const noexcept {
    return smoothed_delta_ > 0.0 ? 1.0 / smoothed_delta_ : 0.0;
}

} // namespace coffee

