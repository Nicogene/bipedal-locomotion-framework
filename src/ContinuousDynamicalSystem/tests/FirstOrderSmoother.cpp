/**
 * @file FirstOrderSmoother.cpp
 * @authors Giulio Romualdi
 * @copyright 2021 Istituto Italiano di Tecnologia (IIT). This software may be modified and
 * distributed under the terms of the GNU Lesser General Public License v2.1 or any later version.
 */

#include <limits>
#include <memory>

// Catch2
#include <catch2/catch.hpp>

#include <Eigen/Dense>

#include <BipedalLocomotion/ContinuousDynamicalSystem/FirstOrderSmoother.h>
#include <BipedalLocomotion/ParametersHandler/StdImplementation.h>

using namespace BipedalLocomotion::ContinuousDynamicalSystem;

TEST_CASE("First order smoother")
{
    constexpr double dT = 0.0001;
    constexpr double settlingTime = 0.1;
    constexpr double tolerance = 1e-2;

    auto params = std::make_shared<BipedalLocomotion::ParametersHandler::StdImplementation>();
    params->setParameter("settling_time", settlingTime);
    params->setParameter("sampling_time", dT);

    FirstOrderSmoother smoother;
    REQUIRE(smoother.initialize(params));

    Eigen::Vector2d initialState = Eigen::Vector2d::Zero();
    Eigen::Vector2d input = Eigen::Vector2d::Ones();

    REQUIRE(smoother.reset(initialState));
    REQUIRE(smoother.setInput(input));

    double settilingTimeSubSystem1 = std::numeric_limits<double>::infinity();
    double settilingTimeSubSystem2 = std::numeric_limits<double>::infinity();
    bool settilingSubSystem1 = false;
    bool settilingSubSystem2 = false;

    // the presented dynamical has the following close form solution in case of step response
    auto closeFormSolution = [settlingTime](const double& t) -> Eigen::Vector2d {
        const double a = 3.0 / settlingTime;
        Eigen::Vector2d sol;
        sol(0) = 1 - std::exp(-a * t);
        sol(1) = 1 - std::exp(-a * t);
        return sol;
    };

    for (unsigned int i = 0; i < 1000; i++)
    {
        const Eigen::Vector2d output = smoother.getOutput();

        // check if the solution is similar to the expected one
        REQUIRE(output.isApprox(closeFormSolution(dT * i), tolerance));

        if (!settilingSubSystem1)
        {
            if ((output[0] > 0.95) && (output[0] < 1.05))
            {
                settilingTimeSubSystem1 = i * dT;
                settilingSubSystem1 = true;
            }
        } else if ((output[0] < 0.95) || (output[0] > 1.05))
        {
            settilingTimeSubSystem1 = std::numeric_limits<double>::infinity();
            settilingSubSystem1 = false;
        }

        if (!settilingSubSystem2)
        {
            if ((output[1] > 0.95) && (output[1] < 1.05))
            {
                settilingTimeSubSystem2 = i * dT;
                settilingSubSystem2 = true;
            }
        } else if ((output[1] < 0.95) || (output[1] > 1.05))
        {
            settilingTimeSubSystem2 = std::numeric_limits<double>::infinity();
            settilingSubSystem2 = false;
        }

        // advance the smoother
        REQUIRE(smoother.advance());
    }

    // check the settling time
    REQUIRE(settilingTimeSubSystem1 <= settlingTime);
    REQUIRE(settilingTimeSubSystem2 <= settlingTime);
}
