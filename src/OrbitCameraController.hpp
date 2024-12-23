#pragma once

#include "Application/Interfaces/ICameraController.h"

/// \c initOrbitCameraController creates a camera that orbits around a fixed
/// point, defined at \c startLookAt.
ICameraController *initOrbitCameraController(const vec3 &startPosition,
                                             const vec3 &startLookAt);
