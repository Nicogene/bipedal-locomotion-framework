/**
 * @file MultiStateWeightProvider.cpp
 * @authors Giulio Romualdi
 * @copyright 2022 Istituto Italiano di Tecnologia (IIT). This software may be modified and
 * distributed under the terms of the GNU Lesser General Public License v2.1 or any later version.
 */

// Catch2
#include <catch2/catch.hpp>

#include <Eigen/Dense>

#include <BipedalLocomotion/ContinuousDynamicalSystem/MultiStateWeightProvider.h>
#include <BipedalLocomotion/ParametersHandler/StdImplementation.h>

using namespace BipedalLocomotion::ContinuousDynamicalSystem;

TEST_CASE("Multistate weight provider")
{
    constexpr double dT = 0.01;
    constexpr double settlingTime = 0.1;
    constexpr double tolerance = 1e-2;

    Eigen::Vector3d walkingWeight;
    walkingWeight << 10, 100, 1000;

    Eigen::Vector3d stanceWeight;
    stanceWeight << 50, 30, 20;

    auto params = std::make_shared<BipedalLocomotion::ParametersHandler::StdImplementation>();
    params->setParameter("settling_time", settlingTime);
    params->setParameter("sampling_time", dT);
    params->setParameter("states", std::vector<std::string>{"WALKING", "STANCE"});

    auto paramsWalking = std::make_shared<BipedalLocomotion::ParametersHandler::StdImplementation>();
    auto paramsStance = std::make_shared<BipedalLocomotion::ParametersHandler::StdImplementation>();

    paramsWalking->setParameter("name", "walking");
    paramsWalking->setParameter("weight", walkingWeight);

    paramsStance->setParameter("name", "stance");
    paramsStance->setParameter("weight", stanceWeight);

    REQUIRE(params->setGroup("WALKING", paramsWalking));
    REQUIRE(params->setGroup("STANCE", paramsStance));

    SECTION("Without reset")
    {
        MultiStateWeightProvider provider;
        REQUIRE(provider.initialize(params));

        REQUIRE(provider.getWeight().isApprox(walkingWeight));
        REQUIRE(provider.setState("stance"));
        REQUIRE_FALSE(provider.setState("foo"));

        for (unsigned int i = 0; i < 50; i++)
        {
            REQUIRE(provider.advance());
        }

        REQUIRE(provider.getWeight().isApprox(stanceWeight, tolerance));
    }

    SECTION("With reset")
    {
        MultiStateWeightProvider provider;
        REQUIRE(provider.initialize(params));

        REQUIRE(provider.getWeight().isApprox(walkingWeight));
        REQUIRE(provider.setState("stance"));

        for (unsigned int i = 0; i < 50; i++)
        {
            REQUIRE(provider.advance());
        }

        REQUIRE(provider.getWeight().isApprox(stanceWeight, tolerance));

        REQUIRE(provider.reset("stance"));
        REQUIRE(provider.getWeight().isApprox(stanceWeight));
        REQUIRE(provider.setState("walking"));
        for (unsigned int i = 0; i < 50; i++)
        {
            REQUIRE(provider.advance());
        }

        REQUIRE(provider.getWeight().isApprox(walkingWeight, tolerance));
    }
}
