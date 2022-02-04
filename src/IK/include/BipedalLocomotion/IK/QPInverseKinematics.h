/**
 * @file QPInverseKinematics.h
 * @authors Giulio Romualdi
 * @copyright 2021 Istituto Italiano di Tecnologia (IIT). This software may be modified and
 * distributed under the terms of the GNU Lesser General Public License v2.1 or any later version.
 */

#ifndef BIPEDAL_LOCOMOTION_IK_QP_INVERSE_KINEMATICS_H
#define BIPEDAL_LOCOMOTION_IK_QP_INVERSE_KINEMATICS_H

#include <memory>
#include <vector>
#include <string>


#include <Eigen/Dense>

#include <BipedalLocomotion/IK/IntegrationBasedIK.h>
#include <BipedalLocomotion/ParametersHandler/IParametersHandler.h>
#include <BipedalLocomotion/System/Advanceable.h>
#include <BipedalLocomotion/System/IWeightProvider.h>

namespace BipedalLocomotion
{

namespace IK
{

/**
 * QPInverseKinematics is a concrete class and implements an integration base inverse kinematics.
 * The inverse kinematics is here implemented as Quadratic Programming (QP) problem. The user should
 * set the desired task with the method QPInverseKinematics::addTask. Each task has a given
 * priority. Currently we support only priority equal to 0 or 1. If the task priority is set to 0
 * the task will be considered as hard task, thus treated as an equality constraint. If the priority
 * is equal to 1 the task will be embedded in the cost function. The class is also able to treat
 * inequality constraints.
 * A possible usage of the IK can be found in "Romualdi et al. A Benchmarking of DCM Based
 * Architectures for Position and Velocity Controlled Walking of Humanoid Robots"
 * https://doi.org/10.1109/HUMANOIDS.2018.8625025
 */
class QPInverseKinematics : public IntegrationBasedIK
{
    /**
     * Private implementation
     */
    struct Impl;
    std::unique_ptr<Impl> m_pimpl;

public:

    /**
     * Constructor.
     */
    QPInverseKinematics();

    /**
     * Destructor.
     */
    virtual ~QPInverseKinematics();

    /**
     * Add a linear task in the solver.
     * @param task pointer to a given linear task
     * @param taskName unique name associated to the task.
     * @param priority Priority associated to the task. The lower the number the higher the
     * priority.
     * @param weight Weight associated to the task.
     * @return true if the task has been added to the inverse kinematics.
     * @note If this method is used the solver assume that the weight is a constant value
     */
    bool addTask(std::shared_ptr<Task> task,
                 const std::string& taskName,
                 std::size_t priority,
                 Eigen::Ref<const Eigen::VectorXd> weight) override;

    /**
     * Add a linear task in the solver.
     * @param task pointer to a given linear task
     * @param taskName unique name associated to the task.
     * @param priority Priority associated to the task. The lower the number the higher the
     * priority.
     * @param weightProvider Weight provider associated to the task. This parameter is optional. The
     * default value is an object that does not contain any value. So is an invalid provider.
     * @return true if the task has been added to the inverse kinematics.
     */
    bool addTask(std::shared_ptr<Task> task,
                 const std::string& taskName,
                 std::size_t priority,
                 std::shared_ptr<const System::IWeightProvider> weightProvider = nullptr) override;

    /**
     * Set the weightProvider associated to an already existing task
     * @param taskName name associated to the task
     * @param weightProvider new Weight provider associated to the task.
     * @return true if the weight has been updated
     */
    bool
    setTaskWeightProvider(const std::string& taskName,
                          std::shared_ptr<const System::IWeightProvider> weightProvider) override;

    /**
     * Get the weightProvider associated to an already existing task
     * @param taskName name associated to the task
     * @return a weak pointer to the weightProvider. If the task does not exist the pointer is not
     * lockable
     */
    std::weak_ptr<const System::IWeightProvider>
    getTaskWeightProvider(const std::string& taskName) const override;

    /**
     * Finalize the IK.
     * @param handler parameter handler.
     * @note You should call this method after you add ALL the tasks.
     * @return true in case of success, false otherwise.
     */
    bool finalize(const System::VariablesHandler& handler) override;


    /**
     * Initialize the inverse kinematics algorithm.
     * @param handler pointer to the IParametersHandler interface.g
     * @note the following parameters are required by the class
     * |         Parameter Name         |   Type   |                                           Description                                          | Mandatory |
     * |:------------------------------:|:--------:|:----------------------------------------------------------------------------------------------:|:---------:|
     * | `robot_velocity_variable_name` | `string` | Name of the variable contained in `VariablesHandler` describing the generalized robot velocity |    Yes    |
     * |           `verbosity`          |  `bool`  |                         Verbosity of the solver. Default value `false`                         |     No    |
     * Where the generalized robot velocity is a vector containing the base spatialvelocity
     (expressed in mixed representation) and the joint velocities.
     * @return True in case of success, false otherwise.
     */
    bool initialize(std::weak_ptr<const ParametersHandler::IParametersHandler> handler) override;

    /**
     * Get a vector containing the name of the tasks.
     * @return an std::vector containing all the names associated to the tasks
     */
    std::vector<std::string> getTaskNames() const override;

    /**
     * Return true if the content of get is valid.
     */
    bool isOutputValid() const override;

    /**
     * Solve the inverse kinematics.
     * @return true in case of success and false otherwise.
     */
    bool advance() override;

    /**
     * Get the outcome of the optimization problem
     * @return the state of the inverse kinematics.
     */
    const State& getOutput() const override;

    /**
     * Get a specific task
     * @param name name associated to the task.
     * @return a weak ptr associated to an existing task in the IK. If the task does not exist a
     * nullptr is returned.
     */
    std::weak_ptr<Task> getTask(const std::string& name) const override;

    /**
     * Return the description of the InverseKinematics problem.
     * @return a string containing the description of the solver.
     */
    std::string toString() const override;

    /**
     * Return the vector representing the entire solution of the QPInverseKinematics.
     * @return a vector containing the solution of the optimization problem
     */
    Eigen::Ref<const Eigen::VectorXd> getRawSolution() const override;
};
} // namespace IK
} // namespace BipedalLocomotion

#endif // BIPEDAL_LOCOMOTION_IK_QP_INVERSE_KINEMATICS_H
