#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>

#include <SDL_audio.h>

#include "detector.hpp"
#include "detector_fsm.hpp"

const double Detector::ft_per_rev = 23.0;

Detector::Detector() :
        fsm_{std::make_unique<DetectorFSM>([this] { dip_callback(); })},
        data_{Data{0, 0.0, std::chrono::steady_clock::now()}} {}

Detector::~Detector() {
    stop();
}

void Detector::start(int audio_device) {
    if (not running_) {
        audio_device_ = audio_device;
        running_ = true;
        thread_ = std::thread(&Detector::run, this);
    }
}

void Detector::stop() {
    if (running_) {
        running_ = false;
        thread_.join();
    }
}

bool Detector::is_running() {
    return running_;
}

void Detector::run() {
    // Unstable past 30mph @ buffer_size = 2048
    constexpr auto buffer_size = 1024;
    constexpr auto max_queued = 1024 * 10;

    auto desired_spec = SDL_AudioSpec{};
    desired_spec.freq = 96000;
    desired_spec.samples = max_queued;
    desired_spec.format = AUDIO_S16;
    desired_spec.channels = 1;
    desired_spec.callback = nullptr;

    auto spec = SDL_AudioSpec{};

    auto device = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(audio_device_, 1), 1, &desired_spec, &spec, 0);
    if (device == 0) {
        std::cout << SDL_GetError() << std::endl;
        return;
    }

    std::array<int16_t, buffer_size> buffer{};

    SDL_PauseAudioDevice(device, 0); // Start capture
    while (running_) {

        if (SDL_GetQueuedAudioSize(device) < buffer_size * 2) {
            // Sleep for ~10ms (960 samples)
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
            continue;
        } else if (SDL_GetQueuedAudioSize(device) > max_queued * 2) {
            std::cout << "not handling audio fast enough" << std::endl;
            SDL_ClearQueuedAudio(device);
            continue;
        }

        auto ret = SDL_DequeueAudio(device, buffer.data(), buffer_size * 2);
        if (ret < buffer_size * 2) {
            std::cout << "Not enough bytes: " << SDL_GetError() << std::endl;
            continue;
        }

        int n = 0;
        auto buffer_mean = 0.0;
        auto buffer_mean_sq_diff = 0.0;
        for (auto&& sample : buffer) {
            n++;
            auto delta = sample - buffer_mean;
            buffer_mean += delta / n;
            auto delta2 = sample - buffer_mean;
            buffer_mean_sq_diff += delta * delta2;
        }

        auto buffer_variance = buffer_mean_sq_diff / (n - 1);
        auto buffer_stdev = std::sqrt(buffer_variance);

        {
            std::lock_guard<std::mutex> lk(mut_);
            fsm_->sm.process_event(FSM::reading{buffer_stdev});
        }
    }

    SDL_CloseAudioDevice(device);
}

Detector::Data Detector::get_data() {
    return data_;
}

bool Detector::is_calibrating() {
    std::lock_guard<std::mutex> lk(mut_);
    return fsm_->sm.is(FSM::Calibrating);
}

void Detector::dip_callback() {
    auto time = std::chrono::steady_clock::now();

    auto data = data_.load();
    auto new_data = Data{++data.count, 0.0, time};

    if (data.count > 0) {
        using Hours = std::chrono::duration<double, std::ratio<3600>>;
        auto delta_t = time - data.time;
        new_data.velocity_mph = ft_per_rev / 5280.0 * 1.0 / std::chrono::duration_cast<Hours>(delta_t).count();
    }

    data_ = new_data;
}

std::chrono::steady_clock::duration Detector::Data::age() const {
    return std::chrono::steady_clock::now() - time;
}

double Detector::Data::instantaneous_velocity_mph() const {
    using Hours = std::chrono::duration<double, std::ratio<3600>>;
    auto possible_velocity_mph =
            Detector::ft_per_rev / 5280.0 * 1.0 / std::chrono::duration_cast<Hours>(age()).count();

    return std::min(velocity_mph, possible_velocity_mph);
}
