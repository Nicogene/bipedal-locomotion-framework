/**
 * @file MultiStateWeightProvider.cpp
 * @authors Giulio Romualdi
 * @copyright 2022 Istituto Italiano di Tecnologia (IIT). This software may be modified and
 * distributed under the terms of the GNU Lesser General Public License v2.1 or any later version.
 */

#include <BipedalLocomotion/ContinuousDynamicalSystem/MultiStateWeightProvider.h>
#include <BipedalLocomotion/TextLogging/Logger.h>

using namespace BipedalLocomotion::ContinuousDynamicalSystem;

Eigen::Ref<const Eigen::VectorXd> MultiStateWeightProvider::getWeight() const
{
    return m_smoother.getOutput();
}

bool MultiStateWeightProvider::initialize(
    std::weak_ptr<const ParametersHandler::IParametersHandler> handler)
{
    constexpr auto logPrefix = "[MultiStateWeightProvider::initialize]";
    if (!m_smoother.initialize(handler))
    {
        log()->error("{} Unable to initialize the smoother.", logPrefix);
        return false;
    }

    std::vector<std::string> states;
    auto ptr = handler.lock();

    if (ptr == nullptr)
    {
        log()->error("{} Invalid parameter handler.", logPrefix);
        return false;
    }

    if (!ptr->getParameter("states", states))
    {
        log()->error("{} Unable to get the parameter named 'states'.", logPrefix);
        return false;
    }

    std::string name;
    for (const auto& state : states)
    {
        auto group = ptr->getGroup(state).lock();
        if (group == nullptr)
        {
            log()->error("{} Unable to get the group named '{}'.", logPrefix, state);
            return false;
        }

        if (!group->getParameter("name", name))
        {
            log()->error("{} Unable to get the parameter named 'name' in the group named '{}'.",
                         logPrefix,
                         state);
            return false;
        }

        if (!group->getParameter("weight", m_states[name]))
        {
            log()->error("{} Unable to get the parameter named 'weight; in the group named '{}'.",
                         logPrefix,
                         state);
            return false;
        }
    }

    std::string firstState;
    ptr->getGroup(states[0]).lock()->getParameter("name", firstState);
    return this->reset(firstState);
}

bool MultiStateWeightProvider::reset(const std::string& state)
{
    constexpr auto logPrefix = "[MultiStateWeightProvider::reset]";

    const auto weight = m_states.find(state);

    if (weight == m_states.end())
    {
        log()->error("{} Unable to find the weight named '{}'", logPrefix, state);
        return false;
    }

    return m_smoother.reset(weight->second);
}

bool MultiStateWeightProvider::setState(const std::string& state)
{
    constexpr auto logPrefix = "[MultiStateWeightProvider::reset]";

    const auto weight = m_states.find(state);
    if (weight == m_states.end())
    {
        log()->error("{} Unable to find the weight named '{}'", logPrefix, state);
        return false;
    }

    return m_smoother.setInput(weight->second);
}

bool MultiStateWeightProvider::advance()
{
    return m_smoother.advance();
}
