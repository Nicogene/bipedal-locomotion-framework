/**
 * @file ParametersHandlerTomlTest.cpp
 * @authors Giulio Romualdi
 * @copyright 2021 Istituto Italiano di Tecnologia (IIT). This software may be modified and
 * distributed under the terms of the GNU Lesser General Public License v2.1 or any later version.
 */

// std
#include <memory>

// Catch2
#include <catch2/catch.hpp>

#include <BipedalLocomotion/GenericContainer/Vector.h>
#include <BipedalLocomotion/ParametersHandler/IParametersHandler.h>
#include <BipedalLocomotion/ParametersHandler/TomlImplementation.h>

using namespace std::string_view_literals;

using namespace BipedalLocomotion::ParametersHandler;
using namespace BipedalLocomotion::GenericContainer;

TEST_CASE("Get parameters")
{
    std::vector<int> fibonacciNumbers{1, 1, 2, 3, 5, 8, 13, 21};
    std::vector<std::string> donaldsNephews{"Huey", "Dewey", "Louie"};
    std::vector<bool> flags{true, false, false, true, true, true};

    static constexpr std::string_view some_toml = R"(
       answer_to_the_ultimate_question_of_life = 42
       pi = 3.14
       John = "Smith"
       "Fibonacci Numbers" = [1, 1, 2, 3, 5, 8, 13, 21]
       flag = true
       flags = [true, false, false, true, true, true])"sv;

    toml::table tbl = toml::parse(some_toml);

    std::shared_ptr<TomlImplementation> originalHandler = std::make_shared<TomlImplementation>(tbl);
    IParametersHandler::shared_ptr parameterHandler = originalHandler;

    SECTION("Get integer")
    {
        int element;
        REQUIRE(parameterHandler->getParameter("answer_to_the_ultimate_question_of_life", element));
        REQUIRE(element == 42);
    }

    SECTION("Get Double")
    {
        double element;
        REQUIRE(parameterHandler->getParameter("pi", element));
        REQUIRE(element == 3.14);
    }

    SECTION("Get Bool")
    {
        bool element;
        REQUIRE(parameterHandler->getParameter("flag", element));
        REQUIRE(element == true);
    }

    SECTION("Get String")
    {
        std::string element;
        REQUIRE(parameterHandler->getParameter("John", element));
        REQUIRE(element == "Smith");
    }

    SECTION("Change String")
    {
        parameterHandler->setParameter("John", "Doe");
        std::string element;
        REQUIRE(parameterHandler->getParameter("John", element));
        REQUIRE(element == "Doe");
    }

    SECTION("Get Vector")
    {
        std::vector<int> element;
        REQUIRE(parameterHandler->getParameter("Fibonacci Numbers", element));
        REQUIRE(element == fibonacciNumbers);
    }

    SECTION("Set bool Vector")
    {
        std::vector<bool> element;
        const std::vector<bool> newFlags{false, true, false, true, true, true, false};
        parameterHandler->setParameter("new_flags", newFlags);
        REQUIRE(parameterHandler->getParameter("new_flags", element));
        REQUIRE(element == newFlags);
    }

    SECTION("Get bool Vector")
    {
        std::vector<bool> element;
        REQUIRE(parameterHandler->getParameter("flags", element));
        REQUIRE(element == flags);
    }

    SECTION("Change Vector")
    {
        fibonacciNumbers.push_back(34);
        parameterHandler->setParameter("Fibonacci Numbers", fibonacciNumbers);
        std::vector<int> element;
        REQUIRE(parameterHandler->getParameter("Fibonacci Numbers", element));
        REQUIRE(element == fibonacciNumbers);
    }

    SECTION("Set/Get Group")
    {
        IParametersHandler::shared_ptr setGroup = std::make_shared<TomlImplementation>();
        setGroup->setParameter("Donald's nephews", donaldsNephews);
        REQUIRE(parameterHandler->setGroup("CARTOONS", setGroup));
        IParametersHandler::shared_ptr cartoonsGroup =
        parameterHandler->getGroup("CARTOONS").lock(); REQUIRE(cartoonsGroup);

        std::vector<std::string> element;
        REQUIRE(cartoonsGroup->getParameter("Donald's nephews", element));
        REQUIRE(element == donaldsNephews);
    }

    SECTION("Print content")
    {
        std::cout << "Parameters: " << parameterHandler->toString() << std::endl;
    }

    SECTION("is Empty")
    {
        IParametersHandler::shared_ptr groupHandler = parameterHandler->getGroup("CARTOONS").lock();
        REQUIRE_FALSE(groupHandler);
        IParametersHandler::shared_ptr setGroup = std::make_shared<TomlImplementation>();
        REQUIRE(parameterHandler->setGroup("CARTOONS", setGroup));

        //now the pointer should be lockable
        groupHandler = parameterHandler->getGroup("CARTOONS").lock();
        REQUIRE(groupHandler); REQUIRE(groupHandler->isEmpty());

        groupHandler->setParameter("Donald's nephews", donaldsNephews);
        REQUIRE_FALSE(groupHandler->isEmpty());
        std::cout << "Parameters: " << parameterHandler->toString() << std::endl;
    }

    SECTION("Clear")
    {
        REQUIRE_FALSE(parameterHandler->isEmpty());
        parameterHandler->clear();
        REQUIRE(parameterHandler->isEmpty());
    }
}
