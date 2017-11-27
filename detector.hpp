#ifndef DETECTOR_HPP
#define DETECTOR_HPP

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

struct DetectorFSM;

class Detector {
public:
    struct Data {
        int count{0};
        double velocity_mph{0.0};
        std::chrono::steady_clock::time_point time{};

        std::chrono::steady_clock::duration age() const;
        double instantaneous_velocity_mph() const;
    };

    Detector();

    ~Detector();

    void start(int audio_device);

    void stop();

    bool is_running();

    bool is_calibrating();

    Data get_data();

    const static double ft_per_rev;

private:
    void run();

    void dip_callback();

    std::thread thread_{};
    std::mutex mut_{};
    std::atomic<bool> running_{false};

    int audio_device_{};
    std::unique_ptr<DetectorFSM> fsm_;
    std::atomic<Data> data_;

};

#endif // DETECTOR_HPP
