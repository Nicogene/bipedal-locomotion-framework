/**
 * @file FirstOrderSmoother.cpp
 * @authors Giulio Romualdi
 * @copyright 2021 Istituto Italiano di Tecnologia (IIT). This software may be modified and
 * distributed under the terms of the GNU Lesser General Public License v2.1 or any later version.
 */

#include <BipedalLocomotion/ContinuousDynamicalSystem/FirstOrderSmoother.h>
#include <BipedalLocomotion/ContinuousDynamicalSystem/LinearTimeInvariantSystem.h>
#include <BipedalLocomotion/TextLogging/Logger.h>

using namespace BipedalLocomotion::ContinuousDynamicalSystem;

bool FirstOrderSmoother::initialize(
    std::weak_ptr<const ParametersHandler::IParametersHandler> handler)
{
    constexpr auto logPrefix = "[FirstOrderSmoother::initialize]";
    auto ptr = handler.lock();

    if (ptr == nullptr)
    {
        log()->error("{} Invalid parameter handler.", logPrefix);
        return false;
    }

    if (!ptr->getParameter("settling_time", m_settlingTime))
    {
        log()->error("{} Unable to get the 'settling_time' parameter.", logPrefix);
        return false;
    }

    double samplingTime{-1};
    if (!ptr->getParameter("sampling_time", samplingTime))
    {
        log()->error("{} Unable to get the 'sampling_time' parameter.", logPrefix);
        return false;
    }
    if (!m_integrator.setIntegrationStep(samplingTime))
    {
        log()->error("{} Unable to set the integration time step.", logPrefix);
        return false;
    }

    m_linearSystem = std::make_shared<LinearTimeInvariantSystem>();
    if (!m_integrator.setDynamicalSystem(m_linearSystem))
    {
        log()->error("{} Unable to initialize integrator.", logPrefix);
        return false;
    }
    m_isInitialized = true;

    return true;
}

bool FirstOrderSmoother::reset(Eigen::Ref<const Eigen::VectorXd> initialPoint)
{
    constexpr auto logPrefix = "[FirstOrderSmoother::reset]";
    m_isInitialStateSet = false;

    if (!m_isInitialized)
    {
        log()->error("{} Please initialize the class before.", logPrefix);
        return false;
    }

    // The following code is required to implement the following linear system
    // dx = -a * x + a * u
    // where a is given by 3.0 / settling_time
    const int systemSize = initialPoint.size();
    const Eigen::MatrixXd A = 3.0 / m_settlingTime //
                              * Eigen::MatrixXd::Identity(systemSize, systemSize);

    if (!m_linearSystem->setSystemMatrices(-A, A))
    {
        log()->error("{} Unable to set the linear system matrices.", logPrefix);
        return false;
    }
    if (!m_linearSystem->setState(initialPoint))
    {
        log()->error("{} Unable to initialize the system.", logPrefix);
        return false;
    }

    m_output = initialPoint;
    m_isInitialStateSet = true;

    return true;
}

bool FirstOrderSmoother::advance()
{

    m_isOutputValid = false;

    if (!m_isInitialized || !m_isInitialStateSet)
    {
        log()->error("[FirstOrderSmoother::advance] Please call initialize() and reset() before "
                     "advance.");
        return false;
    }

    if (!m_integrator.integrate(0, m_integrator.getIntegrationStep()))
    {
        log()->error("[FirstOrderSmoother::advance] Unable to propagate the dynamical system.");
        return false;
    }

    m_output = std::get<0>(m_linearSystem->getState());
    m_isOutputValid = true;

    return true;
}

bool FirstOrderSmoother::setInput(const Eigen::VectorXd& input)
{
    if (!m_isInitialized || !m_isInitialStateSet)
    {
        log()->error("[FirstOrderSmoother::setInput] Please call initialize() and reset() before "
                     "advance.");
        return false;
    }

    return m_linearSystem->setControlInput(input);
}

const Eigen::VectorXd& FirstOrderSmoother::getOutput() const
{
    return m_output;
}

bool FirstOrderSmoother::isOutputValid() const
{
    return m_isOutputValid;
}
