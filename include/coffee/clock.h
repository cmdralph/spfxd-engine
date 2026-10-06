#pragma once

#include <coffee/api.h>

#include <chrono>
#include <algorithm>
#include <cstddef>
#include <utility>

namespace coffee {

class COFFEE_API clock final {
public:
    using underlying_clock = std::chrono::steady_clock;

    clock() noexcept;
    [[nodiscard]] double elapsed_seconds() const noexcept;
    [[nodiscard]] double restart() noexcept;

private:
    underlying_clock::time_point start_;
};

class COFFEE_API frame_timer final {
public:
    explicit frame_timer(double maximum_delta_seconds = 0.25) noexcept;

    double tick() noexcept;
    [[nodiscard]] double delta_seconds() const noexcept;
    [[nodiscard]] double elapsed_seconds() const noexcept;
    [[nodiscard]] double frames_per_second() const noexcept;

private:
    clock clock_;
    double maximum_delta_ = 0.25;
    double delta_ = 0.0;
    double elapsed_ = 0.0;
    double smoothed_delta_ = 0.0;
};

class COFFEE_API fixed_stepper final {
public:
    explicit fixed_stepper(double step_seconds = 1.0 / 60.0,
                           std::size_t maximum_steps_per_frame = 8) noexcept
        : step_(step_seconds > 0.0 ? step_seconds : 1.0 / 60.0),
          maximum_steps_(std::max<std::size_t>(maximum_steps_per_frame, 1)) {}

    template<typename Update>
    std::size_t advance(double frame_delta_seconds, Update&& update) {
        accumulator_ += std::max(0.0, frame_delta_seconds);
        std::size_t steps = 0;
        while (accumulator_ >= step_ && steps < maximum_steps_) {
            std::forward<Update>(update)(step_);
            accumulator_ -= step_;
            ++steps;
        }
        if (steps == maximum_steps_ && accumulator_ >= step_) {
            accumulator_ = 0.0;
        }
        return steps;
    }

    [[nodiscard]] double step_seconds() const noexcept { return step_; }
    [[nodiscard]] double interpolation_alpha() const noexcept { return accumulator_ / step_; }
    void reset() noexcept { accumulator_ = 0.0; }

private:
    double step_ = 1.0 / 60.0;
    double accumulator_ = 0.0;
    std::size_t maximum_steps_ = 8;
};

} // namespace coffee
