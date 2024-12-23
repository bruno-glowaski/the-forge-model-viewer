#include "OrbitCameraController.hpp"

class OrbitCameraController : public ICameraController {
public:
  void setMotionParameters(const CameraMotionParameters &p) override {
    parameters = p;
  }
  void update(float deltaTime) override {
    if (abs(dr) > kEpsilon) {
      currentSpeed += parameters.acceleration * sign(dr) * deltaTime;
    } else {
      float braking = parameters.braking * deltaTime;
      if (braking > abs(currentSpeed)) {
        currentSpeed = 0.0f;
      } else {
        currentSpeed -= braking * sign(currentSpeed);
      }
    }
    if (abs(currentSpeed) > parameters.movementSpeed) {
      currentSpeed = parameters.movementSpeed * sign(currentSpeed);
    }

    vec2 rotationDir = {sign(dphi), sign(dtheta)};
    if (lengthSqr(rotationDir) > 1.0f) {
      rotationDir /= length(rotationDir);
    }
    if (lengthSqr(rotationDir) > kEpsilon) {
      currentRotVel += (parameters.acceleration * deltaTime) * rotationDir;
    } else {
      float rotSpeed = length(currentRotVel);
      if (rotSpeed != 0.0f) {
        float braking = min(parameters.braking * deltaTime, rotSpeed);
        currentRotVel -= (braking / rotSpeed) * currentRotVel;
      }
    }
    if (lengthSqr(currentRotVel) >
        parameters.rotationSpeed * parameters.rotationSpeed) {
      currentRotVel *= parameters.rotationSpeed / length(currentRotVel);
    }

    radius += currentSpeed * deltaTime;
    if (radius < kMinDistance) {
      radius = kMinDistance;
      currentSpeed = 0.0f;
    }

    rotation += currentRotVel * deltaTime;
    float theta = rotation.getY();
    if (theta < kMinTheta || theta > kMaxTheta) {
      currentRotVel.setY(0.0f);
    }
    rotation.setY(clamp(theta, kMinTheta + kEpsilon, kMaxTheta - kEpsilon));

    dr = 0.0f;
    dtheta = 0.0f;
    dphi = 0.0f;
  }

  mat4 getViewMatrix() const override {
    return mat4::lookAtLH(Point3(getViewPosition()), Point3(pivot),
                          vec3::yAxis());
  };
  vec3 getViewPosition() const override {
    float theta = rotation.getY();
    float phi = rotation.getX();
    vec3 dir(cos(theta) * sin(phi), sin(theta), cos(theta) * cos(phi));
    return pivot + (radius * dir);
  };
  vec2 getRotationXY() const override { return rotation; };
  void moveTo(const vec3 &location) override {
    vec3 delta = location - pivot;
    float x = delta.getX();
    float y = delta.getY();
    float z = delta.getZ();
    radius = length(delta);
    float theta = asin(y / radius);
    float phi = asin(x / sqrf(x * x + z * z));
    setViewRotationXY({phi, theta});
  };
  void lookAt(const vec3 &lookAt) override { pivot = lookAt; };
  void setViewRotationXY(const vec2 &v) override {
    rotation = vec2(v.getY(), v.getX());
  };
  void resetView() override {
    pivot = startPivot;
    radius = startRadius;
    rotation = startRotation;
    currentRotVel = startRotation;
  };

  void onMove(const float2 &vec) override {
    dphi += vec.x;
    dr -= vec.y;
  };
  void onMoveY(float y) override { dtheta += y; };
  void onRotate(const float2 &vec) override {
    dtheta -= vec.y;
    dphi += vec.x;
  };
  void onZoom(const float2 &vec) override {
    // Alternatively, this could change the FOV.
    dr += vec.x + vec.y;
  };

  const float kEpsilon = 0.000001f;
  const float kMinTheta = -PI / 2.0f;
  const float kMaxTheta = PI / 2.0f;
  const float kMinDistance = 1.0f;

  vec3 startPivot;
  float startRadius;
  vec2 startRotation;

  vec3 pivot;
  float radius;
  vec2 rotation;

  CameraMotionParameters parameters;
  float currentSpeed;
  vec2 currentRotVel;

private:
  float dr, dtheta, dphi;
};

ICameraController *initOrbitCameraController(const vec3 &startPosition,
                                             const vec3 &startLookAt) {
  OrbitCameraController *cc = tf_placement_new<OrbitCameraController>(
      tf_calloc(1, sizeof(OrbitCameraController)));

  cc->lookAt(startLookAt);
  cc->moveTo(startPosition);

  cc->startPivot = cc->pivot;
  cc->startRadius = cc->radius;
  cc->startRotation = cc->rotation;

  return cc;
}
