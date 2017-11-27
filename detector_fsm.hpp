#ifndef DETECTOR_FSM_HPP
#define DETECTOR_FSM_HPP

#include <chrono>

#include "boost/sml.hpp"

struct CalibrationInfo {
    int readings{0};
    double mean{0.0};
};

namespace FSM {
    constexpr auto calibration_readings = 250;

    // events
    struct reading {
        double stdev;
    };
    struct cal_done {
    };

    // states
    auto Calibrating = boost::sml::state<class Calibrating>;
    auto NoDip = boost::sml::state<class NoDip>;
    auto Dip = boost::sml::state<class Dip>;

    // guards
    auto IsBelowLowerThreshold = [](const reading& e, CalibrationInfo& cal) -> bool {
        // Below 75% mean stdev
        return e.stdev < cal.mean * .75;
    };

    auto IsAboveUpperThreshold = [](const reading& e, CalibrationInfo& cal) -> bool {
        // Above 75% mean stdev
        return e.stdev > cal.mean * .75;
    };

    // actions
    auto AccumulateMean = [](const auto& e, auto& sm, CalibrationInfo& cal) -> void {
        cal.readings++;
        cal.mean += (e.stdev - cal.mean) / cal.readings;

        if (cal.readings == calibration_readings) {
            sm.process_event(cal_done{});
        }
    };

    auto NotifyDip = [](std::function<void(void)>& callback) {
        callback();
    };

    struct Detector_ {
        auto operator()() noexcept {
            using namespace boost::sml;
            return make_transition_table(
                    *Calibrating + event<reading> / AccumulateMean,
                    Calibrating + event<cal_done> = NoDip,

                    NoDip + event<reading>[IsBelowLowerThreshold] / NotifyDip = Dip,

                    Dip + event<reading>[IsAboveUpperThreshold] = NoDip
            );
        }
    };
}

struct DetectorFSM {

    explicit DetectorFSM(std::function<void(void)> dip_callback) :
            callback_(dip_callback),
            sm(cal_, callback_) {}

    CalibrationInfo cal_{};
    std::function<void(void)> callback_{[] {}};

    boost::sml::sm<FSM::Detector_> sm;
};

#endif // DETECTOR_FSM_HPP
