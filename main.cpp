#include <atomic>
#include <chrono>
#include <iostream>
#include <iomanip>

#include <SDL.h>
#include <SDL_audio.h>

#include "detector.hpp"

#include <stdexcept>

class ApplicationError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Application {
public:
    Application() {
        if (SDL_Init(SDL_INIT_AUDIO) != 0) {
            throw (ApplicationError(std::string{"Unable to initialize SDL: "} + std::string{SDL_GetError()}));
        }
    };

    void run();

    ~Application() {
        SDL_Quit();
    };

private:
    // Figure out which audio device to use
    int get_audio_device()
    {
        auto num_devices = SDL_GetNumAudioDevices(1);
        std::cout << num_devices << " inputs" << std::endl;
        for (auto i = 0; i < num_devices; ++i) {
            std::cout << i << ": " << SDL_GetAudioDeviceName(i, 1) << std::endl;
        }

        std::cout << "Which audio device? " << std::flush;

        int audio_device{0};
        std::cin >> audio_device;
        return audio_device;
    }

    Detector detector_{};
};

void Application::run()
{
    detector_.start(get_audio_device());

    std::cout << "Calibrating: do not pedal" << std::endl;
    while (detector_.is_calibrating()) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
    std::cout << std::endl;
    std::cout << "Calibration complete: start pedalling" << std::endl;

    Detector::Data data{};
    data.time = std::chrono::steady_clock::now();

    // Quit after 60 seconds with no update
    while (data.age() < std::chrono::seconds(60)) {
        data = detector_.get_data();
        auto time_since_update = data.age();

        using Hours = std::chrono::duration<double, std::ratio<3600>>;

        // Stop printing distance after 5 seconds with no update
        if (time_since_update < std::chrono::seconds(5)) {
            // Actual distance + anticipated distance
            auto distance_ft = detector_.ft_per_rev * data.count +
                               data.instantaneous_velocity_mph() *
                               std::chrono::duration_cast<Hours>(time_since_update).count() * 5280.0;
            std::cout << static_cast<int>(distance_ft) << " feet (" << std::fixed << std::setprecision(2)
                      << (distance_ft / 5280) << " miles) @ " << data.velocity_mph << " mph" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds{1});
    }

    std::cout << "60 seconds without update, quitting " << std::endl;

    detector_.stop();
}

int main(int argc, char* argv[]) {
    try {
        Application app{};
        app.run();
        return 0;
    } catch (ApplicationError& e) {
        std::cout << e.what() << std::endl;
        return 1;
    };
}
