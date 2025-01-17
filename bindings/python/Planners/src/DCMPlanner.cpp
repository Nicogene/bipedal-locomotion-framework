/**
 * @file DCMPlanner.cpp
 * @authors Diego Ferigo, Giulio Romualdi
 * @copyright 2020 Istituto Italiano di Tecnologia (IIT). This software may be modified and
 * distributed under the terms of the GNU Lesser General Public License v2.1 or any later version.
 */

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <BipedalLocomotion/Planners/DCMPlanner.h>

#include <BipedalLocomotion/bindings/Planners/DCMPlanner.h>
#include <BipedalLocomotion/bindings/System/Advanceable.h>

namespace BipedalLocomotion
{
namespace bindings
{
namespace Planners
{

void CreateDCMPlanner(pybind11::module& module)
{
    namespace py = ::pybind11;
    using namespace BipedalLocomotion::Planners;
    using namespace BipedalLocomotion::System;

    py::class_<DCMPlannerState>(module, "DCMPlannerState")
        .def(py::init())
        .def_readwrite("dcm_position", &DCMPlannerState::dcmPosition)
        .def_readwrite("dcm_velocity", &DCMPlannerState::dcmVelocity)
        .def_readwrite("vrp_position", &DCMPlannerState::vrpPosition)
        .def_readwrite("omega", &DCMPlannerState::omega)
        .def_readwrite("omega_dot", &DCMPlannerState::omegaDot);

    BipedalLocomotion::bindings::System::CreateSource<DCMPlannerState>(module, "DCMPlanner");

    py::class_<DCMPlanner, Source<DCMPlannerState>>(module, "DCMPlanner")
        .def("set_initial_state", &DCMPlanner::setInitialState, py::arg("state"))
        .def("set_contact_phase_list",
             &DCMPlanner::setContactPhaseList,
             py::arg("contact_phase_list"));
}

} // namespace Planners
} // namespace bindings
} // namespace BipedalLocomotion
