#pragma once

#include "hardware/encoder/Encoder.hpp"
#include "hardware/IMU/Imu.hpp"
#include "pros/rtos.hpp"
#include "units/Pose.hpp"
#include <vector>

namespace lemlib {
/**
 * @brief struct representing a tracking wheel
 *
 */
struct TrackingWheel {
        /**
         * @brief Construct a new Tracking Wheel object
         *
         * @param encoder pointer to the encoder which should be used for tracking
         * @param diameter the diameter of the wheel
         * @param offset how far the tracking wheel is from the turning center
         *
         * @b Example:
         * @code {.cpp}
         * // create rotation senor on port 3, which is reversed
         * lemlib::V5RotationSensor encoder(3, true);
         * // create a tracking wheel using the encoder above, which has a wheel diameter
         * // of 2.75 inches and is offset 3 inches to the left
         * lemlib::TrackingWheel trackingWheel(&encoder, 2.75_in, -3_in);
         * @endcode
         */
        TrackingWheel(Encoder* encoder, Length diameter, Length offset);
        Encoder* const encoder;
        const Length diameter;
        const Length offset;
};

/**
 * @brief Tracking Wheel Odometry class
 *
 * This class can be used to track the position a differential drive robot,
 * which has any number of tracking wheels and IMUs, including 0
 */
class TrackingWheelOdometry {
    public:
        /**
         * @brief Construct a new Tracking Wheel Odometry object
         *
         * @param imus vector of Imu pointers
         * @param verticalWheels vector of vertical tracking wheels
         * @param horizontalWheels vector of horizontal tracking wheels
         *
         * @b Example:
         * @code {.cpp}
         * // In this example, the TrackingWheelOdometry class will use the average
         * // value of 2 imus to find the heading of the robot
         * // a single vertical tracking wheel will be used to track the local y
         * // displacement of the robot
         * // the local x displacement of the robot will be assumed to be 0, as
         * // there are no sensors available for this measurement
         *
         * // create IMUs
         * lemlib::V5InertialSensor imu1(1);
         * lemlib::V5InertialSensor imu2(2);
         *
         * // create a vertical tracking wheel
         * lemlib::V5RotationSensor verticalEncoder(3, false);
         * lemlib::TrackingWheel verticalWheel(&verticalEncoder, 2.75_in, 2_in);
         *
         * // no horizontal tracking wheels for this example
         *
         * // create a TrackingWheelOdometry instance
         * lemlib::TrackingWheelOdometry odom({imu1, imu2}, {verticalWheel}, {});
         * @endcode
         *
         * @b Example:
         * @code {.cpp}
         * // in this example, the TrackingWheelOdometry class will calculate
         * // the heading using 2 horizontal tracking wheels
         * // a single vertical tracking wheel will be used to calculate the local
         * // y displacement of the robot
         * // a single horizontal tracking wheel will be used to calculate the local
         * // x displacement of the robot
         *
         * // no IMUs for this example
         *
         * // create a vertical tracking wheel
         * lemlib::V5RotationSensor verticalEncoder(1, false);
         * lemlib::TrackingWheel verticalWheel(&verticalEncoder, 2.75_in, 2_in);
         *
         * // create 2 horizontal tracking wheels
         * lemlib::ADIEncoder horizontalEncoder1('a', 'b', true);
         * lemlib::ADIEncoder2 horizontalEncoder2(2, 'c', 'd', false);
         * lemlib::TrackingWheel horizontalWheel1(&horizontalEncoder1, 3.25_in, -2_in);
         * lemlib::TrackingWheel horizontalWheel2(&horizontalEncoder2, 2.75_in, -4_in);
         *
         * // create a TrackingWheelOdometry instance
         * lemlib::TrackingWheelOdometry odom({}, {verticalWheel}, {horizontalWheel1, horizontalWheel2});
         * @endcode
         */
        TrackingWheelOdometry(std::vector<Imu*> imus, std::vector<TrackingWheel> verticalWheels,
                              std::vector<TrackingWheel> horizontalWheels);
        /**
         * @brief calibrate the sensors and start the tracking task
         *
         * @param maxCalibrationTime the maximum time that can be spent calibrating. Defaults to 3 seconds
         *
         * @return 0 if no errors occurred during calibration
         * @return 1 if a calibration error occurred, but a subsequent attempt was successful
         * @return 2 if a calibration error occurred, but a substitute sensor will be used
         * @return 3 if a calibration error occurred, but an alternative heading tracking method will be used
         * @return 4 if a calibration error occurred, so the local x and/or y axis can't be tracked
         * @return 5 if a calibration error occurred, so the heading can't be tracked
         *
         * @b Example:
         * @code {.cpp}
         * TrackingWheelOdometry odom(...);
         *
         * void initialize() {
         *   // calibrate the sensors used for tracking
         *   // try to calibrate for up to 2 seconds
         *   odom.calibrate(2_sec);
         *   // the code below will run as soon as the calibration is successful,
         *   // or the timer has run out
         *   std::cout << "Calibration Complete" << std::endl;
         * }
         * @endcode
         */
        int calibrate(Time maxCalibrationTime = 3_sec);
        /**
         * @brief Get the estimated Pose of the robot
         *
         * @return units::Pose the estimated pose
         *
         * @b Example:
         * // create TrackingWheelOdom object
         * lemlib::TrackingWheelOdom odom(...);
         *
         * @code {.cpp}
         * void initialize() {
         *   pros::lcd::initialize(); // initialize brain screen
         *   odom.calibrate(); // calibrate odom
         *   while (true) {
         *     // get current pose of the robot
         *     const lemlib::Pose pose = odom.getPose();
         *     // print pros to the brain screen
         *     pros::lcd::print(0, "x: %f", to_in(pose.x));
         *     pros::lcd::print(1, "y: %f", to_in(pose.y));
         *     pros::lcd::print(2, "theta: %f", to_in(pose.theta));
         *     // delay to let other tasks run
         *     pros::delay(10);
         *   }
         * }
         * @endcode
         */
        units::Pose getPose();
        /**
         * @brief Set the estimated pose of the robot
         *
         * @param pose the new pose
         *
         * @b Example:
         * @code {.cpp}
         * // create TrackingWheelOdom object
         * lemlib::TrackingWheelOdom odom(...);
         *
         * void autonomous() {
         *   // set the starting position of the robot
         *   odom.setPose({15_in, -12_in, 90_cDeg});
         * }
         * @endcode
         */
        void setPose(units::Pose pose);
    private:
        /**
         * @brief update the estimated pose
         *
         * This function should have its own dedicated task
         */
        void update();
        std::optional<pros::Task> m_task = std::nullopt;
        std::vector<Imu*> m_Imus;
        std::vector<TrackingWheel> m_verticalWheels;
        std::vector<TrackingWheel> m_horizontalWheels;
};
} // namespace lemlib