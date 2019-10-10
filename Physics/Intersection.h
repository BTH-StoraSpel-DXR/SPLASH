#pragma once

#include "BoundingBox.h"
#include "Cylinder.h"
#include "Sphere.h"

struct Frustum;

class Intersection {
public:
	static bool AabbWithAabb(const BoundingBox& aabb1, const BoundingBox& aabb2);
	static bool AabbWithTriangle(const BoundingBox& aabb, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3);
	static bool AabbWithPlane(const BoundingBox& aabb, const glm::vec3& normal, const float& distance);
	static bool AabbWithSphere(BoundingBox& aabb, const Sphere& sphere);
	static bool AabbWithVerticalCylinder(BoundingBox& aabb, const VerticalCylinder& cyl);

	static bool TriangleWithTriangle(const glm::vec3 U[3], const glm::vec3 V[3]);
	static bool TriangleWithSphere(const glm::vec3 tri[3], const Sphere& sphere);
	static bool TriangleWithVerticalCylinder(const glm::vec3 tri[3], const VerticalCylinder& cyl);

	static bool PointWithVerticalCylinder(const glm::vec3 p, const VerticalCylinder& cyl);

	static bool LineSegmentWithVerticalCylinder(const glm::vec3& start, const glm::vec3& end, const VerticalCylinder& cyl);

	static float RayWithAabb(const glm::vec3& rayStart, const glm::vec3& rayVec, const BoundingBox& aabb);
	static float RayWithTriangle(const glm::vec3& rayStart, const glm::vec3& rayDir, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3);
	static float RayWithPlane(const glm::vec3& rayStart, const glm::vec3& rayDir, const glm::vec3& normal, const float& distance);
	static float RayWithPaddedAabb(const glm::vec3& rayStart, const glm::vec3& rayVec, const BoundingBox& aabb, float padding);
	static float RayWithPaddedTriangle(const glm::vec3& rayStart, const glm::vec3& rayDir, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, float padding);

	static bool FrustumWithAabb(const Frustum& frustum, const BoundingBox& aabb);
private:
	//Private constructor so an instance can't be created
	Intersection() {};
	~Intersection() {};

	static BoundingBox sPaddedReserved; //Used in RayWithPaddedAabb to avoid having to create a new bounding box every time

	static bool TriangleWithTriangleSupport(const glm::vec3 U[3], const glm::vec3 V[3], glm::vec3 outSegment[2]);
	static void Barycentric(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float& u, float& v, float& w);
	static bool OnTriangle(const float u, const float v, const float w);
};
