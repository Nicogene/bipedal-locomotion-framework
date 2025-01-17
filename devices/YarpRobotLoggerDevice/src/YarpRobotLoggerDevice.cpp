/**
 * @copyright 2020, 2021 Istituto Italiano di Tecnologia (IIT). This software may be modified and
 * distributed under the terms of the GNU Lesser General Public License v2.1 or any later version.
 */

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <memory>
#include <tuple>

#include <BipedalLocomotion/ParametersHandler/IParametersHandler.h>
#include <BipedalLocomotion/ParametersHandler/YarpImplementation.h>
#include <BipedalLocomotion/System/Clock.h>
#include <BipedalLocomotion/System/YarpClock.h>
#include <BipedalLocomotion/TextLogging/Logger.h>
#include <BipedalLocomotion/TextLogging/LoggerBuilder.h>
#include <BipedalLocomotion/TextLogging/YarpLogger.h>
#include <BipedalLocomotion/YarpRobotLoggerDevice.h>
#include <BipedalLocomotion/YarpUtilities/Helper.h>
#include <BipedalLocomotion/YarpUtilities/VectorsCollection.h>

#include <yarp/os/BufferedPort.h>
#include <yarp/telemetry/experimental/BufferConfig.h>

#include <matioCpp/ForwardDeclarations.h>
#include <matioCpp/Span.h>

using namespace BipedalLocomotion::YarpUtilities;
using namespace BipedalLocomotion::ParametersHandler;
using namespace BipedalLocomotion::RobotInterface;
using namespace BipedalLocomotion;

YarpRobotLoggerDevice::YarpRobotLoggerDevice(double period,
                                             yarp::os::ShouldUseSystemClock useSystemClock)
    : yarp::os::PeriodicThread(period, useSystemClock)
{
    // Use the yarp clock in blf
    BipedalLocomotion::System::ClockBuilder::setFactory(
        std::make_shared<BipedalLocomotion::System::YarpClockFactory>());

    // the logging message are streamed using yarp
    BipedalLocomotion::TextLogging::LoggerBuilder::setFactory(
        std::make_shared<BipedalLocomotion::TextLogging::YarpLoggerFactory>());
}

YarpRobotLoggerDevice::YarpRobotLoggerDevice()
    : yarp::os::PeriodicThread(0.01, yarp::os::ShouldUseSystemClock::No)
{
    // Use the yarp clock in blf
    BipedalLocomotion::System::ClockBuilder::setFactory(
        std::make_shared<BipedalLocomotion::System::YarpClockFactory>());

    // the logging message are streamed using yarp
    BipedalLocomotion::TextLogging::LoggerBuilder::setFactory(
        std::make_shared<BipedalLocomotion::TextLogging::YarpLoggerFactory>());
}

YarpRobotLoggerDevice::~YarpRobotLoggerDevice() = default;

bool YarpRobotLoggerDevice::open(yarp::os::Searchable& config)
{
    auto params = std::make_shared<ParametersHandler::YarpImplementation>(config);

    double devicePeriod{0.01};
    if (params->getParameter("sampling_period_in_s", devicePeriod))
    {
        this->setPeriod(devicePeriod);
    }

    if (!this->setupRobotSensorBridge(params->getGroup("RobotSensorBridge")))
    {
        return false;
    }

    if (!this->setupTelemetry(params->getGroup("Telemetry"), devicePeriod))
    {
        return false;
    }

    if (!this->setupExogenousInputs(params->getGroup("ExogenousSignals")))
    {
        return false;
    }

    return true;
}

bool YarpRobotLoggerDevice::setupExogenousInputs(
    std::weak_ptr<const ParametersHandler::IParametersHandler> params)
{
    constexpr auto logPrefix = "[YarpRobotLoggerDevice::setupExogenousInputs]";

    auto ptr = params.lock();
    if (ptr == nullptr)
    {
        log()->info("{} No exogenous input will be logged.", logPrefix);
        return true;
    }

    std::vector<std::string> inputs;
    if (!ptr->getParameter("exogenous_inputs", inputs))
    {
        log()->error("{} Unable to get the exogenous inputs.", logPrefix);
        return false;
    }

    for (const auto& input : inputs)
    {
        auto group = ptr->getGroup(input).lock();
        std::string portName, signalName;
        if (group == nullptr || !group->getParameter("port_name", portName)
            || !group->getParameter("signal_name", signalName))
        {
            log()->error("{} Unable to get the parameters related to the input: {}.",
                         logPrefix,
                         input);
            return false;
        }

        if (!m_exogenousPorts[signalName].open(portName))
        {
            log()->error("{} Unable to open the port named: {}.", logPrefix, portName);
            return false;
        }
    }

    return true;
}

bool YarpRobotLoggerDevice::setupTelemetry(
    std::weak_ptr<const ParametersHandler::IParametersHandler> params, const double& devicePeriod)
{
    constexpr auto logPrefix = "[YarpRobotLoggerDevice::setupTelemetry]";

    auto ptr = params.lock();
    if (ptr == nullptr)
    {
        log()->error("{} The parameters handler is not valid.", logPrefix);
        return false;
    }

    yarp::telemetry::experimental::BufferConfig config;
    config.yarp_robot_name = std::getenv("YARP_ROBOT_NAME");
    config.filename = "robot_logger_device";
    config.auto_save = true;
    config.save_periodically = true;
    config.file_indexing = "%Y_%m_%d_%H_%M_%S";
    config.mat_file_version = matioCpp::FileVersion::MAT7_3;

    if (!ptr->getParameter("save_period", config.save_period))
    {
        log()->error("{} Unable to get the 'save_period' parameter for the telemetry.", logPrefix);
        return false;
    }

    // the telemetry will flush the content of its storage every config.save_period
    // and this device runs every devicePeriod
    // so the size of the telemetry buffer must be at least config.save_period / devicePeriod
    // to be sure we are not going to lose data the buffer will be 10% longer
    constexpr double percentage = 0.1;
    config.n_samples = static_cast<int>(std::ceil((1 + percentage) //
                                                  * (config.save_period / devicePeriod)));

    return m_bufferManager.configure(config);
}

bool YarpRobotLoggerDevice::setupRobotSensorBridge(
    std::weak_ptr<const ParametersHandler::IParametersHandler> params)
{
    constexpr auto logPrefix = "[YarpRobotLoggerDevice::setupRobotSensorBridge]";

    auto ptr = params.lock();
    if (ptr == nullptr)
    {
        log()->error("{} The parameters handler is not valid.", logPrefix);
        return false;
    }

    m_robotSensorBridge = std::make_unique<YarpSensorBridge>();
    if (!m_robotSensorBridge->initialize(ptr))
    {
        log()->error("{} Unable to configure the 'SensorBridge'", logPrefix);
        return false;
    }

    // Get additional flags required by the device
    if (!ptr->getParameter("stream_joint_states", m_streamJointStates))
    {
        log()->info("{} The 'stream_joint_states' parameter is not found. The joint states is not "
                    "logged",
                    logPrefix);
    }

    if (!ptr->getParameter("stream_motor_states", m_streamMotorStates))
    {
        log()->info("{} The 'stream_motor_states' parameter is not found. The motor states is not "
                    "logged",
                    logPrefix);
    }

    if (!ptr->getParameter("stream_motor_PWM", m_streamMotorPWM))
    {
        log()->info("{} The 'stream_motor_PWM' parameter is not found. The motor PWM is not logged",
                    logPrefix);
    }

    if (!ptr->getParameter("stream_pids", m_streamPIDs))
    {
        log()->info("{} The 'stream_pids' parameter is not found. The motor pid values are not "
                    "logged",
                    logPrefix);
    }

    if (!ptr->getParameter("stream_inertials", m_streamInertials))
    {
        log()->info("{} The 'stream_inertials' parameter is not found. The IMU values are not "
                    "logged",
                    logPrefix);
    }

    if (!ptr->getParameter("stream_cartesian_wrenches", m_streamCartesianWrenches))
    {
        log()->info("{} The 'stream_cartesian_wrenches' parameter is not found. The cartesian "
                    "wrench values are not "
                    "logged",
                    logPrefix);
    }

    if (!ptr->getParameter("stream_forcetorque_sensors", m_streamFTSensors))
    {
        log()->info("{} The 'stream_forcetorque_sensors' parameter is not found. The FT values are "
                    "not "
                    "logged",
                    logPrefix);
    }

    if (!ptr->getParameter("stream_temperatures", m_streamTemperatureSensors))
    {
        log()->info("{} The 'stream_temperatures' parameter is not found. The temperature sensor "
                    "values are not "
                    "logged",
                    logPrefix);
    }

    return true;
}

bool YarpRobotLoggerDevice::attachAll(const yarp::dev::PolyDriverList& poly)
{
    constexpr auto logPrefix = "[YarpRobotLoggerDevice::attachAll]";

    if (!m_robotSensorBridge->setDriversList(poly))
    {
        log()->error("{} Could not attach drivers list to sensor bridge.", logPrefix);
        return false;
    }

    // TODO this should be removed
    // this sleep is required since the sensor bridge could be not ready
    using namespace std::chrono_literals;
    BipedalLocomotion::clock().sleepFor(2000ms);

    std::vector<std::string> joints;
    if (!m_robotSensorBridge->getJointsList(joints))
    {
        log()->error("{} Could not get the joints list.", logPrefix);
        return false;
    }

    const unsigned dofs = joints.size();
    m_bufferManager.setDescriptionList(joints);

    bool ok = true;

    // prepare the telemetry
    if (m_streamJointStates)
    {
        ok = ok && m_bufferManager.addChannel({"joints_state::positions", {dofs, 1}, joints});
        ok = ok && m_bufferManager.addChannel({"joints_state::velocities", {dofs, 1}, joints});
        ok = ok && m_bufferManager.addChannel({"joints_state::accelerations", {dofs, 1}, joints});
        ok = ok && m_bufferManager.addChannel({"joints_state::torques", {dofs, 1}, joints});
    }
    if (m_streamMotorStates)
    {
        ok = ok && m_bufferManager.addChannel({"motors_state::positions", {dofs, 1}, joints});
        ok = ok && m_bufferManager.addChannel({"motors_state::velocities", {dofs, 1}, joints});
        ok = ok && m_bufferManager.addChannel({"motors_state::accelerations", {dofs, 1}, joints});
        ok = ok && m_bufferManager.addChannel({"motors_state::currents", {dofs, 1}, joints});
    }

    if (m_streamMotorPWM)
    {
        ok = ok && m_bufferManager.addChannel({"motors_state::PWM", {dofs, 1}, joints});
    }

    if (m_streamPIDs)
    {
        ok = ok && m_bufferManager.addChannel({"PIDs", {dofs, 1}, joints});
    }

    if (m_streamFTSensors)
    {
        for (const auto& sensorName : m_robotSensorBridge->getSixAxisForceTorqueSensorsList())
        {
            ok = ok
                 && m_bufferManager.addChannel({"FTs::" + sensorName,
                                                {6, 1}, //
                                                {"f_x", "f_y", "f_y", "mu_x", "mu_y", "mu_y"}});
        }
    }

    if (m_streamInertials)
    {
        for (const auto& sensorName : m_robotSensorBridge->getGyroscopesList())
        {
            ok = ok
                 && m_bufferManager.addChannel({"gyros::" + sensorName,
                                                {3, 1}, //
                                                {"omega_x", "omega_y", "omega_z"}});
        }

        for (const auto& sensorName : m_robotSensorBridge->getLinearAccelerometersList())
        {
            ok = ok
                 && m_bufferManager.addChannel({"accelerometers::" + sensorName,
                                                {3, 1}, //
                                                {"a_x", "a_y", "a_z"}});
        }

        for (const auto& sensorName : m_robotSensorBridge->getOrientationSensorsList())
        {
            ok = ok
                 && m_bufferManager.addChannel({"orientations::" + sensorName,
                                                {3, 1}, //
                                                {"r", "p", "y"}});
        }

        // an IMU contains a gyro accelerometer and an orientation sensor
        for (const auto& sensorName : m_robotSensorBridge->getIMUsList())
        {
            ok = ok
                 && m_bufferManager.addChannel({"accelerometers::" + sensorName,
                                                {3, 1}, //
                                                {"a_x", "a_y", "a_z"}})
                 && m_bufferManager.addChannel({"gyros::" + sensorName,
                                                {3, 1}, //
                                                {"omega_x", "omega_y", "omega_z"}})
                 && m_bufferManager.addChannel({"orientations::" + sensorName,
                                                {3, 1}, //
                                                {"r", "p", "y"}});
        }
    }

    if (m_streamCartesianWrenches)
    {
        for (const auto& sensorName : m_robotSensorBridge->getCartesianWrenchesList())
        {
            ok = ok
                 && m_bufferManager.addChannel({"cartesian_wrenches::" + sensorName,
                                                {6, 1}, //
                                                {"f_x", "f_y", "f_y", "mu_x", "mu_y", "mu_y"}});
        }
    }

    if (m_streamTemperatureSensors)
    {
        for (const auto& sensorName : m_robotSensorBridge->getTemperatureSensorsList())
        {
            ok = ok
                 && m_bufferManager.addChannel({"temperatures::" + sensorName,
                                                {1, 1}, //
                                                {"temperature"}});
        }
    }

    // resize the temporary vectors
    m_jointSensorBuffer.resize(dofs);

    if (ok)
    {
        return start();
    }
    return ok;
}

void YarpRobotLoggerDevice::unpackIMU(Eigen::Ref<const analog_sensor_t> signal,
                                      Eigen::Ref<accelerometer_t> accelerometer,
                                      Eigen::Ref<gyro_t> gyro,
                                      Eigen::Ref<orientation_t> orientation)
{
    // the output consists 12 double, organized as follows:
    //  euler angles [3]
    // linear acceleration [3]
    // angular speed [3]
    // magnetic field [3]
    // http://wiki.icub.org/wiki/Inertial_Sensor
    orientation = signal.segment<3>(0);
    accelerometer = signal.segment<3>(3);
    gyro = signal.segment<3>(6);
}

void YarpRobotLoggerDevice::run()
{
    constexpr auto logPrefix = "[YarpRobotLoggerDevice::run]";

    // get the data
    if (!m_robotSensorBridge->advance())
    {
        log()->error("{} Could not advance sensor bridge.", logPrefix);
    }

    const double time = BipedalLocomotion::clock().now().count();

    // collect the data
    if (m_streamJointStates)
    {
        if (m_robotSensorBridge->getJointPositions(m_jointSensorBuffer))
        {
            m_bufferManager.push_back(m_jointSensorBuffer, time, "joints_state::positions");
        }
        if (m_robotSensorBridge->getJointVelocities(m_jointSensorBuffer))
        {
            m_bufferManager.push_back(m_jointSensorBuffer, time, "joints_state::velocities");
        }
        if (m_robotSensorBridge->getJointAccelerations(m_jointSensorBuffer))
        {
            m_bufferManager.push_back(m_jointSensorBuffer, time, "joints_state::accelerations");
        }
        if (m_robotSensorBridge->getJointTorques(m_jointSensorBuffer))
        {
            m_bufferManager.push_back(m_jointSensorBuffer, time, "joints_state::torques");
        }
    }

    if (m_streamMotorStates)
    {
        if (m_robotSensorBridge->getMotorPositions(m_jointSensorBuffer))
        {
            m_bufferManager.push_back(m_jointSensorBuffer, time, "motors_state::positions");
        }
        if (m_robotSensorBridge->getMotorVelocities(m_jointSensorBuffer))
        {
            m_bufferManager.push_back(m_jointSensorBuffer, time, "motors_state::velocities");
        }
        if (m_robotSensorBridge->getMotorAccelerations(m_jointSensorBuffer))
        {
            m_bufferManager.push_back(m_jointSensorBuffer, time, "motors_state::accelerations");
        }
        if (m_robotSensorBridge->getMotorCurrents(m_jointSensorBuffer))
        {
            m_bufferManager.push_back(m_jointSensorBuffer, time, "motors_state::currents");
        }
    }

    if (m_streamMotorPWM)
    {
        if (m_robotSensorBridge->getMotorPWMs(m_jointSensorBuffer))
        {
            m_bufferManager.push_back(m_jointSensorBuffer, time, "motors_state::PWM");
        }
    }

    if (m_streamPIDs)
    {
        if (m_robotSensorBridge->getPidPositions(m_jointSensorBuffer))
        {
            m_bufferManager.push_back(m_jointSensorBuffer, time, "PIDs");
        }
    }

    for (const auto& sensorName : m_robotSensorBridge->getSixAxisForceTorqueSensorsList())
    {
        if (m_robotSensorBridge->getSixAxisForceTorqueMeasurement(sensorName, m_ftBuffer))
        {
            m_bufferManager.push_back(m_ftBuffer, time, "FTs::" + sensorName);
        }
    }

    for (const auto& sensorname : m_robotSensorBridge->getTemperatureSensorsList())
    {
        if (m_robotSensorBridge->getTemperature(sensorname, m_ftTemperatureBuffer))
        {
            m_bufferManager.push_back({m_ftTemperatureBuffer}, time, "temperatures::" + sensorname);
        }
    }

    for (const auto& sensorName : m_robotSensorBridge->getGyroscopesList())
    {
        if (m_robotSensorBridge->getGyroscopeMeasure(sensorName, m_gyroBuffer))
        {
            m_bufferManager.push_back(m_gyroBuffer, time, "gyros::" + sensorName);
        }
    }

    for (const auto& sensorName : m_robotSensorBridge->getLinearAccelerometersList())
    {
        if (m_robotSensorBridge->getLinearAccelerometerMeasurement(sensorName,
                                                                   m_acceloremeterBuffer))
        {
            m_bufferManager.push_back(m_acceloremeterBuffer, time, "accelerometers::" + sensorName);
        }
    }

    for (const auto& sensorName : m_robotSensorBridge->getOrientationSensorsList())
    {
        if (m_robotSensorBridge->getOrientationSensorMeasurement(sensorName, m_orientationBuffer))
        {
            m_bufferManager.push_back(m_orientationBuffer, time, "orientations::" + sensorName);
        }
    }

    // an IMU contains a gyro accelerometer and an orientation sensor
    for (const auto& sensorName : m_robotSensorBridge->getIMUsList())
    {
        if (m_robotSensorBridge->getIMUMeasurement(sensorName, m_analogSensorBuffer))
        {
            // it will return a tuple containing the Accelerometer, the gyro and the orientatio
            this->unpackIMU(m_analogSensorBuffer,
                            m_acceloremeterBuffer,
                            m_gyroBuffer,
                            m_orientationBuffer);

            m_bufferManager.push_back(m_acceloremeterBuffer, time, "accelerometers::" + sensorName);
            m_bufferManager.push_back(m_gyroBuffer, time, "gyros::" + sensorName);
            m_bufferManager.push_back(m_orientationBuffer, time, "orientations::" + sensorName);
        }
    }

    for (const auto& sensorName : m_robotSensorBridge->getCartesianWrenchesList())
    {
        if (m_robotSensorBridge->getCartesianWrench(sensorName, m_ftBuffer))
        {
            m_bufferManager.push_back(m_ftBuffer, time, "cartesian_wrenches::" + sensorName);
        }
    }

    std::string signalFullName;
    for (auto& [name, port] : m_exogenousPorts)
    {
        BipedalLocomotion::YarpUtilities::VectorsCollection* collection = port.read(false);
        if (collection != nullptr)
        {
            for (const auto& [key, vector] : collection->vectors)
            {
                signalFullName = name + "::" + key;

                // if it is the first time this signal is seen by the device the channel is added
                if (m_exogenousPortsStoredInManager.find(signalFullName)
                    == m_exogenousPortsStoredInManager.end())
                {
                    m_bufferManager.addChannel({signalFullName, {vector.size(), 1}});
                    m_exogenousPortsStoredInManager.insert(signalFullName);
                }

                m_bufferManager.push_back(vector, time, signalFullName);
            }
        }
    }
}

bool YarpRobotLoggerDevice::detachAll()
{
    if (isRunning())
    {
        stop();
    }

    return true;
}

bool YarpRobotLoggerDevice::close()
{
    return true;
}
