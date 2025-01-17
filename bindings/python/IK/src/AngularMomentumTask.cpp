/**
 * @file AngularMomentumTask.cpp
 * @authors Giulio Romualdi
 * @copyright 2021 Istituto Italiano di Tecnologia (IIT). This software may be modified and
 * distributed under the terms of the GNU Lesser General Public License v2.1 or any later version.
 */

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <BipedalLocomotion/IK/IKLinearTask.h>
#include <BipedalLocomotion/IK/AngularMomentumTask.h>
#include <BipedalLocomotion/bindings/IK/AngularMomentumTask.h>

namespace BipedalLocomotion
{
namespace bindings
{
namespace IK
{

void CreateAngularMomentumTask(pybind11::module& module)
{
    namespace py = ::pybind11;
    using namespace BipedalLocomotion::IK;

    py::class_<AngularMomentumTask, //
               std::shared_ptr<AngularMomentumTask>,
               IKLinearTask>(module, "AngularMomentumTask")
        .def(py::init())
        .def("set_kin_dyn", &AngularMomentumTask::setKinDyn, py::arg("kin_dyn"))
        .def("set_set_point", &AngularMomentumTask::setSetPoint, py::arg("angular_momentum"));
}

} // namespace IK
} // namespace bindings
} // namespace BipedalLocomotion
