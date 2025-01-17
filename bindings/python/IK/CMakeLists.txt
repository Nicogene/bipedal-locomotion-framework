# Copyright (C) 2021 Istituto Italiano di Tecnologia (IIT). All rights reserved.
# This software may be modified and distributed under the terms of the
# GNU Lesser General Public License v2.1 or any later version.

if(FRAMEWORK_COMPILE_IK)

  set(H_PREFIX include/BipedalLocomotion/bindings/IK)

  add_bipedal_locomotion_python_module(
    NAME IKBindings
    SOURCES src/IntegrationBasedIK.cpp src/QPInverseKinematics.cpp src/CoMTask.cpp src/SE3Task.cpp src/SO3Task.cpp src/JointTrackingTask.cpp src/AngularMomentumTask.cpp src/Module.cpp src/IKLinearTask.cpp
    HEADERS ${H_PREFIX}/IntegrationBasedIK.h ${H_PREFIX}/QPInverseKinematics.h ${H_PREFIX}/CoMTask.h ${H_PREFIX}/SE3Task.h ${H_PREFIX}/SO3Task.h ${H_PREFIX}/JointTrackingTask.h ${H_PREFIX}/AngularMomentumTask.h ${H_PREFIX}/Module.h ${H_PREFIX}/IKLinearTask.h
    LINK_LIBRARIES BipedalLocomotion::IK
    TESTS tests/test_QP_inverse_kinematics.py
    TESTS_RUNTIME_CONDITIONS FRAMEWORK_USE_icub-models
    )

endif()
