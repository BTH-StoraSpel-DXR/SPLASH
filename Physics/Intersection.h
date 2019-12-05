#pragma once

#include "BoundingBox.h"
#include "Cylinder.h"
#include "Sphere.h"
#include "Sail/graphics/camera/Frustum.h"

class Intersection {
public:
	static bool AabbWithAabb(const glm::vec3& aabb1Pos, const glm::vec3& aabb1HalfSize, const glm::vec3& aabb2Pos, const glm::vec3& aabb2HalfSize);
	static bool AabbWithAabb(const glm::vec3& aabb1Pos, const glm::vec3& aabb1HalfSize, const glm::vec3& aabb2Pos, const glm::vec3& aabb2HalfSize, glm::vec3* intersectionAxis, float* intersectionDepth);
	static bool AabbWithTriangle(const glm::vec3& aabbPos, const glm::vec3& aabbHalfSize, const glm::vec3& triPos1, const glm::vec3& triPos2, const glm::vec3& triPos3, const bool checkBackfaces = false);
	static bool AabbWithTriangle(const glm::vec3& aabbPos, const glm::vec3& aabbHalfSize, const glm::vec3& triPos1, const glm::vec3& triPos2, const glm::vec3& triPos3, glm::vec3* intersectionAxis, float* intersectionDepth);
	static bool AabbWithPlane(const glm::vec3* aabbCorners, const glm::vec3& planeNormal, const float planeDistance);
	static bool AabbWithSphere(const glm::vec3* aabbCorners, const Sphere& sphere);
	static bool AabbWithVerticalCylinder(const glm::vec3& aabbPos, const glm::vec3& aabbHalfSize, const glm::vec3* aabbCorners, const VerticalCylinder& cyl);

	static bool TriangleWithTriangle(const glm::vec3 U[3], const glm::vec3 V[3]);
	static bool TriangleWithSphere(const glm::vec3 tri[3], const Sphere& sphere);
	static bool TriangleWithVerticalCylinder(const glm::vec3 tri[3], const VerticalCylinder& cyl);

	static bool SphereWithPlane(const Sphere& sphere, const glm::vec3& planeNormal, const float planeDistance);

	static bool PointWithVerticalCylinder(const glm::vec3 p, const VerticalCylinder& cyl);

	static bool LineSegmentWithVerticalCylinder(const glm::vec3& start, const glm::vec3& end, const VerticalCylinder& cyl);

	static float RayWithAabb(const glm::vec3& rayStart, const glm::vec3& rayVec, const glm::vec3& aabbPos, const glm::vec3& aabbHalfSize, glm::vec3* intersectionAxis = nullptr);
	static float RayWithTriangle(const glm::vec3& rayStart, const glm::vec3& rayDir, const glm::vec3& triPos1, const glm::vec3& triPos2, const glm::vec3& triPos3);
	static float RayWithPlane(const glm::vec3& rayStart, const glm::vec3& rayDir, const glm::vec3& planeNormal, const float planeDistance);
	static float RayWithPaddedAabb(const glm::vec3& rayStart, const glm::vec3& rayVec, const glm::vec3& aabbPos, const glm::vec3& aabbHalfSize, float padding, glm::vec3* intersectionAxis = nullptr);
	static float RayWithPaddedTriangle(const glm::vec3& rayStart, const glm::vec3& rayDir, const glm::vec3& triPos1, const glm::vec3& triPos2, const glm::vec3& triPos3, float padding, const bool checkBackfaces = false);

	static bool FrustumPlaneWithAabb(const glm::vec3& planeNormal, const float planeDistance, const glm::vec3* aabbCorners);
	static bool FrustumWithAabb(const Frustum& frustum, const glm::vec3* aabbCorners);

	static glm::vec3 PointProjectedOnPlane(const glm::vec3& point, const glm::vec3& planeNormal, const float planeDistance);
private:
	//Private constructor so an instance can't be created
	Intersection() {};
	~Intersection() {};

	static bool TriangleWithTriangleSupport(const glm::vec3 U[3], const glm::vec3 V[3], glm::vec3 outSegment[2]);
	static void Barycentric(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float& outU, float& outV, float& outW);
	static bool OnTriangle(const float u, const float v, const float w);

	static bool SATTest(const glm::vec3& testAxis, const glm::vec3& triPos1, const glm::vec3& triPos2, const glm::vec3& triPos3, const glm::vec3& aabbHalfSize, glm::vec3* intersectionAxis, float* depth);
};
