// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban
// =============================================================================
//
// Base class for a tire.
// A tire subsystem is a force element. It is passed position and velocity
// information of the wheel body and it produces ground reaction forces and
// moments to be applied to the wheel body.
//
// =============================================================================

#include "physics/ChSystem.h"

#include "chrono_vehicle/wheeled_vehicle/ChTire.h"

namespace chrono {
namespace vehicle {

ChTire::ChTire(const std::string& name) : m_name(name) {
}

// -----------------------------------------------------------------------------
// Base class implementation of the initialization function.
// -----------------------------------------------------------------------------
void ChTire::Initialize(std::shared_ptr<ChBody> wheel, VehicleSide side) {
    m_wheel = wheel;
    m_side = side;
}

// -----------------------------------------------------------------------------
// Utility function for characterizing the geometric contact between a disc with
// specified center location, normal direction, and radius and the terrain,
// assumed to be specified as a height field (over the x-y domain).
// This function returns false if no contact occurs. Otherwise, it sets the
// contact points on the disc (ptD) and on the terrain (ptT), the normal contact
// direction, and the resulting penetration depth (a positive value).
// -----------------------------------------------------------------------------
bool ChTire::disc_terrain_contact(const ChTerrain& terrain,
                                  const ChVector<>& disc_center,
                                  const ChVector<>& disc_normal,
                                  double disc_radius,
                                  ChCoordsys<>& contact,
                                  double& depth) {
    // Find terrain height below disc center. There is no contact if the disc
    // center is below the terrain or farther away by more than its radius.
    double hc = terrain.GetHeight(disc_center.x, disc_center.y);
    if (disc_center.z <= hc || disc_center.z >= hc + disc_radius)
        return false;

    // Find the lowest point on the disc. There is no contact if the disc is
    // (almost) horizontal.
    ChVector<> dir1 = Vcross(disc_normal, ChVector<>(0, 0, 1));
    double sinTilt2 = dir1.Length2();

    if (sinTilt2 < 1e-3)
        return false;

    // Contact point (lowest point on disc).
    ChVector<> ptD = disc_center + disc_radius * Vcross(disc_normal, dir1 / sqrt(sinTilt2));

    // Find terrain height at lowest point. No contact if lowest point is above
    // the terrain.
    double hp = terrain.GetHeight(ptD.x, ptD.y);

    if (ptD.z > hp)
        return false;

    // Approximate the terrain with a plane. Define the projection of the lowest
    // point onto this plane as the contact point on the terrain.
    ChVector<> normal = terrain.GetNormal(ptD.x, ptD.y);
    ChVector<> longitudinal = Vcross(disc_normal, normal);
    longitudinal.Normalize();
    ChVector<> lateral = Vcross(normal, longitudinal);
    ChMatrix33<> rot;
    rot.Set_A_axis(longitudinal, lateral, normal);

    contact.pos = ptD;
    contact.rot = rot.Get_A_quaternion();

    depth = Vdot(ChVector<>(0, 0, hp - ptD.z), normal);
    assert(depth > 0);

    return true;
}

}  // end namespace vehicle
}  // end namespace chrono
