/**
 * @file YarpLogger.h
 * @authors Giulio Romualdi
 * @copyright 2021 Istituto Italiano di Tecnologia (IIT). This software may be modified and
 * distributed under the terms of the GNU Lesser General Public License v2.1 or any later version.
 */

#ifndef BIPEDAL_LOCOMOTION_TEXT_LOGGING_YARP_LOGGER_H
#define BIPEDAL_LOCOMOTION_TEXT_LOGGING_YARP_LOGGER_H

#include <memory>
#include <mutex>
#include <string>

#include <spdlog/common.h>
#include <spdlog/sinks/base_sink.h>

#include <BipedalLocomotion/TextLogging/Logger.h>

#include <yarp/os/LogStream.h>

namespace BipedalLocomotion
{
namespace TextLogging
{

namespace sinks
{

template <typename Mutex> class YarpSink : public spdlog::sinks::base_sink<Mutex>
{
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {

        // log_msg is a struct containing the log entry info like level, timestamp, thread id etc.
        // msg.raw contains pre formatted log
        spdlog::memory_buf_t formatted;

        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

        std::string formattedString(formatted.data(), formatted.size());

        if (msg.level == spdlog::level::level_enum::trace)
        {
            yTrace() << formattedString;
        } else if (msg.level == spdlog::level::level_enum::debug)
        {
            yDebug() << formattedString;
        } else if (msg.level == spdlog::level::level_enum::info)
        {
            yInfo() << formattedString;
        } else if (msg.level == spdlog::level::level_enum::warn)
        {
            yWarning() << formattedString;
        } else if (msg.level == spdlog::level::level_enum::err)
        {
            yError() << formattedString;
        } else
        {
            yFatal() << formattedString;
        }
    }

    void flush_() override
    {
    }
};

using YarpSink_mt = YarpSink<std::mutex>;
} // namespace sinks


/**
 * YarpLoggetFactory implements the factory you should use to enable the sink using yaro.
 * The YARP logger can be easily used as follows
 * \code{.cpp}
 * #include <BipedalLocomotion/TextLogging/Logger.h>
 * #include <BipedalLocomotion/TextLogging/YarpLogger.h>
 * #include <BipedalLocomotion/TextLogging/LoggerBuilder.h>
 *
 * // Change the logger
 * BipedalLocomotion::TextLogging::LoggerBuilder::setFactory(std::make_shared<BipedalLocomotion::TextLogging::YarpLoggerFactory>()));
 *
 * BipedalLocomotion::log()->info("My info");
 * \endcode
 */
class YarpLoggerFactory final : public LoggerFactory
{
public:
    /**
     * Create the YARPLogger as a singleton
     * @return the pointer to TextLogging::Logger that streams the output using YARP
     */
    TextLogging::Logger* const createLogger() final;
};

} // namespace TextLogging
} // namespace BipedalLocomotion

#endif // BIPEDAL_LOCOMOTION_TEXT_LOGGING_YARP_LOGGER_H
