#include "PhysicsPCH.h"
#include <algorithm>

#include "Intersection.h"
#include "Sail/graphics/camera/Camera.h"

bool Intersection::AabbWithAabb(const glm::vec3& aabb1Pos, const glm::vec3& aabb1HalfSize, const glm::vec3& aabb2Pos, const glm::vec3& aabb2HalfSize) {
	if (glm::abs(aabb1Pos.x - aabb2Pos.x) > (aabb1HalfSize.x + aabb2HalfSize.x)) {
		return false;
	}
	if (glm::abs(aabb1Pos.y - aabb2Pos.y) > (aabb1HalfSize.y + aabb2HalfSize.y)) {
		return false;
	}
	if (glm::abs(aabb1Pos.z - aabb2Pos.z) > (aabb1HalfSize.z + aabb2HalfSize.z)) {
		return false;
	}
	return true;
}

bool Intersection::AabbWithAabb(const glm::vec3& aabb1Pos, const glm::vec3& aabb1HalfSize, const glm::vec3& aabb2Pos, const glm::vec3& aabb2HalfSize, glm::vec3* intersectionAxis, float* intersectionDepth) {
	*intersectionDepth = 9999999.0f;

	float tempDepth;
	for (int i = 0; i < 3; i++) {
		tempDepth = (aabb1HalfSize[i] + aabb2HalfSize[i]) - glm::abs(aabb1Pos[i] - aabb2Pos[i]);
		if (tempDepth < 0.0f) {
			return false;
		}
		else if (tempDepth < *intersectionDepth) {
			*intersectionDepth = tempDepth;
			*intersectionAxis = glm::vec3(0.0f);
			(*intersectionAxis)[i] = aabb1Pos[i] - aabb2Pos[i];
			(*intersectionAxis)[i] /= abs((*intersectionAxis)[i]); //Normalize
		}
	}
	return true;
}

bool Intersection::AabbWithTriangle(const glm::vec3& aabbPos, const glm::vec3& aabbHalfSize, const glm::vec3& triPos1, const glm::vec3& triPos2, const glm::vec3& triPos3, const bool checkBackfaces) {
	//Calculate normal for triangle
	glm::vec3 triNormal = glm::normalize(glm::cross(glm::vec3(triPos1 - triPos2), glm::vec3(triPos1 - triPos3)));

	// Calculate triangle points relative to the AABB
	glm::vec3 newV1 = triPos1 - aabbPos;
	glm::vec3 newV2 = triPos2 - aabbPos;
	glm::vec3 newV3 = triPos3 - aabbPos;

	//Don't intersect with triangles facing away from the boundingBox
	if (glm::dot(newV1, triNormal) > 0.0f && !checkBackfaces) {
		return false;
	}

	// Calculate the plane that the triangle is on
	glm::vec3 triangleToWorldOrigo = glm::vec3(0.0f) - triPos1;
	float distance = -glm::dot(triangleToWorldOrigo, triNormal);

	// Early exit: Check if a sphere around aabb intersects triangle plane
	if (SphereWithPlane({ aabbPos, glm::length(aabbHalfSize) }, triNormal, distance)) {
		// Testing AABB with triangle using separating axis theorem(SAT)
		glm::vec3 e[3];
		e[0] = glm::vec3(1.f, 0.f, 0.f);
		e[1] = glm::vec3(0.f, 1.f, 0.f);
		e[2] = glm::vec3(0.f, 0.f, 1.f);

		glm::vec3 f[3];
		f[0] = newV2 - newV1;
		f[1] = newV3 - newV2;
		f[2] = newV1 - newV3;

		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				glm::vec3 a = glm::cross(e[i], f[j]);
				glm::vec3 p = glm::vec3(glm::dot(a, newV1), glm::dot(a, newV2), glm::dot(a, newV3));
				float r = aabbHalfSize.x * glm::abs(a.x) + aabbHalfSize.y * glm::abs(a.y) + aabbHalfSize.z * glm::abs(a.z);
				if (glm::min(p.x, glm::min(p.y, p.z)) > r || glm::max(p.x, glm::max(p.y, p.z)) < -r) {
					return false;
				}
			}
		}
	}
	else {
		return false;
	}

	return true;
}

bool Intersection::AabbWithTriangle(const glm::vec3& aabbPos, const glm::vec3& aabbHalfSize, const glm::vec3& triPos1, const glm::vec3& triPos2, const glm::vec3& triPos3, glm::vec3* intersectionAxis, float* intersectionDepth) {
	//This version sets the intersection axis for the smallest collision and the intersection depth along that axis.
	float depth = 9999999.0f;
	glm::vec3 tempIntersectionAxis;

	//Calculate normal for triangle
	glm::vec3 triNormal = glm::normalize(glm::cross(glm::vec3(triPos1 - triPos2), glm::vec3(triPos1 - triPos3)));

	// Calculate triangle points relative to the AABB
	glm::vec3 newV1 = triPos1 - aabbPos;
	glm::vec3 newV2 = triPos2 - aabbPos;
	glm::vec3 newV3 = triPos3 - aabbPos;

	//Don't intersect with triangles faceing away from the boundingBox
	if (glm::dot(newV1, triNormal) > 0.0f) {
		return false;
	}

	// Calculate the plane that the triangle is on
	glm::vec3 triangleToWorldOrigo = glm::vec3(0.0f) - triPos1;
	float distance = -glm::dot(triangleToWorldOrigo, triNormal);

	// Early exit: Check if a sphere around aabb intersects triangle plane
	if (SphereWithPlane({ aabbPos, glm::length(aabbHalfSize) }, triNormal, distance)) {
		// Testing AABB with triangle using separating axis theorem(SAT)
		glm::vec3 e[3];
		e[0] = glm::vec3(1.f, 0.f, 0.f);
		e[1] = glm::vec3(0.f, 1.f, 0.f);
		e[2] = glm::vec3(0.f, 0.f, 1.f);

		glm::vec3 f[3];
		f[0] = glm::normalize(newV2 - newV1);
		f[1] = glm::normalize(newV3 - newV2);
		f[2] = glm::normalize(newV1 - newV3);

		glm::vec3 testAxis;

		for (int i = 0; i < 3; i++) {
			testAxis = e[i];

			if (!SATTest(testAxis, newV1, newV2, newV3, aabbHalfSize, &tempIntersectionAxis, &depth)) {
				return false;
			}

			for (int j = 0; j < 3; j++) {
				testAxis = glm::normalize(glm::cross(e[i], f[j]));

				if (!SATTest(testAxis, newV1, newV2, newV3, aabbHalfSize, &tempIntersectionAxis, &depth)) {
					return false;
				}
			}
		}

		testAxis = triNormal;

		if (!SATTest(testAxis, newV1, newV2, newV3, aabbHalfSize, &tempIntersectionAxis, &depth)) {
			return false;
		}

	}
	else {
		return false;
	}


	//Return intersection axis and depth if not nullptr
	if (intersectionAxis) {
		glm::vec3 triMiddle = (triPos1 + triPos2 + triPos3) / 3.0f;

		if (glm::dot(aabbPos - triMiddle, tempIntersectionAxis) < 0.0f) {
			tempIntersectionAxis = -tempIntersectionAxis;
		}

		*intersectionAxis = tempIntersectionAxis;
	}

	if (intersectionDepth) {
		*intersectionDepth = depth;
	}

	return true;
}

bool Intersection::AabbWithPlane(const glm::vec3* aabbCorners, const glm::vec3& planeNormal, const float planeDistance) {
	const float distFromPlaneAlongNormal[] = {
		glm::dot(aabbCorners[0], planeNormal) - planeDistance,
		glm::dot(aabbCorners[1], planeNormal) - planeDistance,
		glm::dot(aabbCorners[2], planeNormal) - planeDistance,
		glm::dot(aabbCorners[3], planeNormal) - planeDistance,
		glm::dot(aabbCorners[4], planeNormal) - planeDistance,
		glm::dot(aabbCorners[5], planeNormal) - planeDistance,
		glm::dot(aabbCorners[6], planeNormal) - planeDistance,
		glm::dot(aabbCorners[7], planeNormal) - planeDistance
	};

	// Find smallest and biggest distance (opposing corners)
	float minDist = distFromPlaneAlongNormal[0];
	float maxDist = distFromPlaneAlongNormal[0];
	for (short i = 1; i < 8; i++) {
		minDist = std::fminf(minDist, distFromPlaneAlongNormal[i]);
		maxDist = std::fmaxf(maxDist, distFromPlaneAlongNormal[i]);
	}
	// True if they are on opposite sides of the plane
	return minDist * maxDist < 0.0f;
}

bool Intersection::AabbWithSphere(const glm::vec3* aabbCorners, const Sphere& sphere) {
	// Find the point on the aabb closest to the sphere
	float closestOnAabbX = std::fminf(std::fmaxf(sphere.position.x, aabbCorners[0].x), aabbCorners[1].x);
	float closestOnAabbY = std::fminf(std::fmaxf(sphere.position.y, aabbCorners[2].y), aabbCorners[0].y);	// corners[0] > corners[2]
	float closestOnAabbZ = std::fminf(std::fmaxf(sphere.position.z, aabbCorners[0].z), aabbCorners[4].z);

	// Distance from cylinder to closest point on rectangle
	float distX = closestOnAabbX - sphere.position.x;
	float distY = closestOnAabbY - sphere.position.y;
	float distZ = closestOnAabbZ - sphere.position.z;
	float distSquared = distX * distX + distY * distY + distZ * distZ;

	// True if the distance is smaller than the radius
	return (distSquared < sphere.radius * sphere.radius);
}

bool Intersection::AabbWithVerticalCylinder(const glm::vec3& aabbPos, const glm::vec3& aabbHalfSize, const glm::vec3* aabbCorners, const VerticalCylinder& cyl) {
	float yPosDifference = aabbPos.y - cyl.position.y;
	float halfHeightSum = aabbHalfSize.y + cyl.halfHeight;

	// Check if the objects are too far from each other along the y-axis
	if (halfHeightSum < std::fabsf(yPosDifference)) {
		return false;
	}

	// Only 2D calculations below

	// Find the point on the aabb closest to the cylinder
	float closestOnAabbX = std::fminf(std::fmaxf(cyl.position.x, aabbCorners[0].x), aabbCorners[1].x);
	float closestOnAabbZ = std::fminf(std::fmaxf(cyl.position.z, aabbCorners[0].z), aabbCorners[4].z);

	// Distance from cylinder to closest point on rectangle
	float distX = closestOnAabbX - cyl.position.x;
	float distZ = closestOnAabbZ - cyl.position.z;
	float distSquared = distX * distX + distZ * distZ;

	// True if the distance is smaller than the radius
	return (distSquared < cyl.radius * cyl.radius);
}

bool Intersection::TriangleWithTriangle(const glm::vec3 U[3], const glm::vec3 V[3]) {
	glm::vec3 S0[2], S1[2];
	if (TriangleWithTriangleSupport(V, U, S0) && TriangleWithTriangleSupport(U, V, S1))
	{
		// Theoretically, the segments lie on the same line.  A direction D
		// of the line is the Cross(NormalOf(U),NormalOf(V)).  We choose the
		// average A of the segment endpoints as the line origin.
		glm::vec3 uNormal = glm::cross(U[1] - U[0], U[2] - U[0]);
		glm::vec3 vNormal = glm::cross(V[1] - V[0], V[2] - V[0]);
		glm::vec3 D = glm::normalize(glm::cross(uNormal, vNormal));
		glm::vec3 A = 0.25f * (S0[0] + S0[1] + S1[0] + S1[1]);

		// Each segment endpoint is of the form A + t*D.  Compute the
		// t-values to obtain I0 = [t0min,t0max] for S0 and I1 = [t1min,t1max]
		// for S1.  The segments intersect when I0 overlaps I1.  Although this
		// application acts as a "test intersection" query, in fact the
		// construction here is a "find intersection" query.
		float t00 = glm::dot(D, S0[0] - A), t01 = glm::dot(D, S0[1] - A);
		float t10 = glm::dot(D, S1[0] - A), t11 = glm::dot(D, S1[1] - A);
		auto I0 = std::minmax(t00, t01);
		auto I1 = std::minmax(t10, t11);
		return (I0.second > I1.first&& I0.first < I1.second);
	}
	return false;
}

bool Intersection::TriangleWithSphere(const glm::vec3 tri[3], const Sphere& sphere) {
	// Calculations found at http://realtimecollisiondetection.net/blog/?p=103
	// NOTE: in the comments of the article above, it is mentioned that this test is fast,
	// but does not necessarily have perfect precision

	const glm::vec3 A = tri[0] - sphere.position;
	const glm::vec3 B = tri[1] - sphere.position;
	const glm::vec3 C = tri[2] - sphere.position;
	const float rr = sphere.radius * sphere.radius;
	const glm::vec3 V = glm::cross(B - A, C - A);
	const float d = glm::dot(A, V);
	const float e = glm::dot(V, V);

	if (d * d > rr* e) {
		return false;
	}

	const float aa = glm::dot(A, A);
	const float ab = glm::dot(A, B);
	const float ac = glm::dot(A, C);
	const float bb = glm::dot(B, B);
	const float bc = glm::dot(B, C);
	const float cc = glm::dot(C, C);

	if (aa > rr&& ab > aa&& ac > aa) {
		return false;
	}
	if (bb > rr&& ab > bb&& bc > bb) {
		return false;
	}
	if (cc > rr&& ac > cc&& bc > cc) {
		return false;
	}

	const glm::vec3 AB = B - A;
	const glm::vec3 BC = C - B;
	const glm::vec3 CA = A - C;

	const float d1 = ab - aa;
	const float d2 = bc - bb;
	const float d3 = ab - cc;
	const float e1 = glm::dot(AB, AB);
	const float e2 = glm::dot(BC, BC);
	const float e3 = glm::dot(CA, CA);

	const glm::vec3 Q1 = A * e1 - d1 * AB;
	const glm::vec3 Q2 = B * e2 - d2 * BC;
	const glm::vec3 Q3 = C * e3 - d3 * CA;
	const glm::vec3 QC = C * e1 - Q1;
	const glm::vec3 QA = A * e2 - Q2;
	const glm::vec3 QB = B * e3 - Q3;

	if ((glm::dot(Q1, Q1) > rr* e1* e1) && (glm::dot(Q1, QC) > 0)) {
		return false;
	}
	if ((glm::dot(Q2, Q2) > rr* e2* e2) && (glm::dot(Q2, QA) > 0)) {
		return false;
	}
	if ((glm::dot(Q3, Q3) > rr* e3* e3) && (glm::dot(Q3, QB) > 0)) {
		return false;
	}

	return true;
}

bool Intersection::TriangleWithVerticalCylinder(const glm::vec3 tri[3], const VerticalCylinder& cyl) {
	if (PointWithVerticalCylinder(tri[0], cyl)) {
		return true;
	}
	if (PointWithVerticalCylinder(tri[1], cyl)) {
		return true;
	}
	if (PointWithVerticalCylinder(tri[2], cyl)) {
		return true;
	}
	if (LineSegmentWithVerticalCylinder(tri[0], tri[1], cyl)) {
		return true;
	}
	if (LineSegmentWithVerticalCylinder(tri[0], tri[2], cyl)) {
		return true;
	}
	if (LineSegmentWithVerticalCylinder(tri[1], tri[2], cyl)) {
		return true;
	}



	/*
		NOTE:
		These tests are NOT enough to guarantee a collision
		More will need to be done
	*/

	return false;
}

bool Intersection::SphereWithPlane(const Sphere& sphere, const glm::vec3& planeNormal, const float planeDistance) {
	const glm::vec3 pointOnPlane = planeNormal * planeDistance;
	const glm::vec3 centerToPlane = pointOnPlane - sphere.position;
	const float shortestDistanceToPlane = glm::dot(centerToPlane, planeNormal);
	return (std::fabsf(shortestDistanceToPlane) < sphere.radius);
}

bool Intersection::PointWithVerticalCylinder(const glm::vec3 p, const VerticalCylinder& cyl) {
	float distY = p.y - cyl.position.y;

	// Check if point is above or below cylinder
	if (std::fabsf(distY) > cyl.halfHeight) {
		return false;
	}

	float distX = p.x - cyl.position.x;
	float distZ = p.z - cyl.position.z;

	// Check if point is within radius of cylinder
	return (distX * distX + distZ * distZ < cyl.radius * cyl.radius);
}

bool Intersection::LineSegmentWithVerticalCylinder(const glm::vec3& start, const glm::vec3& end, const VerticalCylinder& cyl) {

	// Check if either start or end are within the cylinder
	// This simplifies calculations below, but is unnecessary if the caller has already checked them
	if (PointWithVerticalCylinder(start, cyl) || PointWithVerticalCylinder(end, cyl)) {
		return true;
	}

	/*
		Then check if the intersections are within the line segment
		Then check if the y-coordinates are within the cylinder
	*/

	glm::vec3 dir = glm::normalize(end - start);
	glm::vec3 toRay = start - cyl.position;

	float t_0;
	float t_1;

	// Check against an infinitely tall cylinder
	{
		// Project rays in 2D
		glm::vec2 dir2D = glm::normalize(glm::vec2(dir.x, dir.z));
		glm::vec2 toStart2D = glm::vec2(start.x, start.z) - glm::vec2(cyl.position.x, cyl.position.z);

		// Code found at http://viclw17.github.io/2018/07/16/raytracing-ray-sphere-intersection/
		float a = glm::dot(dir, dir);
		float b = 2.0f * glm::dot(toStart2D, dir2D);
		float c = glm::dot(toStart2D, toStart2D) - (cyl.radius * cyl.radius);
		float d = b * b - 4.0f * a * c;

		// Check if the line misses the infinite cylinder (ray misses the circle)
		if (d < 0.0f) {
			return false;
		}

		// Calculate the distance to the two intersections (in 2D)
		float sq = std::sqrtf(d);
		float divA = 1.0f / (2.0f * a);
		t_0 = (-b - sq) * divA;		// t_0 is always smaller than t_1 (see code above)
		t_1 = (-b + sq) * divA;
	}


	// Distance from start to end
	float t_end = glm::length(end - start);

	// Cylinder top and bottom
	float lowY = cyl.position.y - cyl.halfHeight;
	float highY = cyl.position.y + cyl.halfHeight;

	// Intersection heights
	float y_0 = start.y + dir.y * t_0;
	float y_1 = start.y + dir.y * t_1;

	// Determine which of the two intersections are within the line segment
	// Then check if the intersections are within the cylinder height
	bool isColliding = false;
	bool checkFurther = false;
	if (t_0 < 0.0f) {
		if (t_1 < 0.0f) {
			// Both intersections are "behind" the segment
			// Collision is impossible
		}
		else if (t_1 < t_end) {
			// First intersection is "behind" the segment, second is "within"

			// Check if the second intersection is within the cylinder height
			if (lowY < y_1 && y_1 < highY) {
				isColliding = true;
			}
		}
		else {
			// First intersection is "behind" the segment, second is "in front of"
			// Neither start nor end are within the cylinder (first thing checked in this function)
			// This means the points of the segment are one of three cases
			// 1. both above (Collision is impossible)
			// 2. both below (Collision is impossible)
			// 3. one above and one below

			// Check whether both points are either above or below
			// NOTE: this has to check the POINTS, not the intersections
			//     If one intersection is above and the other within or below, the segment (both points) can still be above (among other cases)
			bool bothAboveOrBelow = (start.y > highY&& end.y > highY) || (start.y < lowY && end.y < lowY);
			if (!bothAboveOrBelow) {
				isColliding = true;
			}
		}
	}
	else if (t_0 < t_end) {
		if (t_1 < t_end) {
			// Both intersections are "within" the segment

			// Check if either 
			if ((lowY < y_0 && y_0 < highY) || (lowY < y_1 && y_1 < highY)) {
				isColliding = true;
			}
		}
		else {
			// First intersection is "within" the segment, second is "in front of"
			if (lowY < y_0 && y_0 < highY) {
				isColliding = true;
			}
		}
	}
	else {
		// Both intersections are "in front of" the segment
		// Collision is impossible
	}

	return isColliding;
}

float Intersection::RayWithAabb(const glm::vec3& rayStart, const glm::vec3& rayVec, const glm::vec3& aabbPos, const glm::vec3& aabbHalfSize, glm::vec3* intersectionAxis) {
	float returnValue = -1.0f;
	glm::vec3 normalizedRay = glm::normalize(rayVec);
	bool noHit = false; //Boolean for early exits from the for-loop
	float tMin = -std::numeric_limits<float>::infinity(); //tMin initialized at negative infinity
	float tMax = std::numeric_limits<float>::infinity(); //tMax initialized at positive infinity
	glm::vec3 minAxis(0.0f);
	glm::vec3 p = aabbPos - rayStart; //Vector to center off AABB
	for (int i = 0; i < 3; i++) {
		float tempH = aabbHalfSize[i]; //Temporary variable to store the current half axis
		glm::vec3 tempAxis(0.0f);
		tempAxis[i] = tempH;

		float e = p[i];
		float f = normalizedRay[i];
		if (f != 0.0f) { //Ray is not parallel to slab
			float tempF = 1 / f; //temporary variable to avoid calculating division with the (possibly) very small value f multiple times since it is an expensive calculation

			//Finds the entering and exiting points of the current slab.
			float t1 = (e + tempH) * tempF;
			float t2 = (e - tempH) * tempF;
			if (t1 > t2) { //Swaps values if t1 is bigger than t2
				float tempT = t2;
				t2 = t1;
				t1 = tempT;
				tempAxis = -tempAxis;
			}
			if (t1 > tMin) {//Replaces tMin if t1 is bigger
				tMin = t1;
				minAxis = tempAxis;
			}
			if (t2 < tMax) {//Replaces tMax if t2 is smaller
				tMax = t2;
			}
			if (tMin > tMax || tMax < 0) { //tMin > tMax - A slab was exited before all slabs had been entered. tMax < 0 - the AABB is "behind" the ray start
				//Exit test, no hit.
				i = 3;
				noHit = true;
			}
		}
		else if (-e - tempH > 0 || -e + tempH < 0) { //Ray is parallel to slab and it does not go through OBB
			//Exit test, no hit.
			i = 3;
			noHit = true;
		}
	}

	if (noHit == false) { //The AABB was hit by the ray
		if (tMin > 0) { //tMin is bigger than 0 so it is the first intersection
			returnValue = tMin;
		}
		else if (tMax > 0) { //tMin is smaller than 0 so try with tMax.
			returnValue = 0.0f; //Ray started in AABB, instant hit. Distance is 0
		}
	}

	if (intersectionAxis) {
		*intersectionAxis = glm::normalize(minAxis);
	}

	return returnValue;
}

float Intersection::RayWithTriangle(const glm::vec3& rayStart, const glm::vec3& rayDir, const glm::vec3& triPos1, const glm::vec3& triPos2, const glm::vec3& triPos3) {
	const glm::vec3 edge1 = triPos2 - triPos1;
	const glm::vec3 edge2 = triPos3 - triPos1;
	const glm::vec3 planeNormal = glm::normalize(glm::cross(edge1, edge2));

	const float originToPlaneDistance = glm::dot(triPos1, planeNormal);
	const float rayToPlaneDistance = RayWithPlane(rayStart, rayDir, planeNormal, originToPlaneDistance);

	if (rayToPlaneDistance == -1.0f) {
		return -1.0f;
	}

	// Determine barycentric coordinates u, v, w
	float u, v, w;
	const glm::vec3 p = rayStart + rayDir * rayToPlaneDistance;
	Barycentric(p, triPos1, triPos2, triPos3, u, v, w);

	// Check if point on triangle plane is within triangle
	if (OnTriangle(u, v, w)) {
		return rayToPlaneDistance;
	}
	return -1.0f;
}

float Intersection::RayWithPlane(const glm::vec3& rayStart, const glm::vec3& rayDir, const glm::vec3& planeNormal, const float planeDistance) {
	const float dirDotNormal = glm::dot(rayDir, planeNormal);

	bool isParallelWithPlane = std::fabsf(dirDotNormal) < 0.001f;
	if (isParallelWithPlane) {
		return -1.0f;
	}

	const glm::vec3 pointOnPlane = planeNormal * planeDistance;
	const glm::vec3 startToPlane = pointOnPlane - rayStart;
	const float shortestDistanceToPlane = glm::dot(startToPlane, planeNormal);
	const float distanceToPlane = shortestDistanceToPlane / dirDotNormal;

	return distanceToPlane;
}

float Intersection::RayWithPaddedAabb(const glm::vec3& rayStart, const glm::vec3& rayVec, const glm::vec3& aabbPos, const glm::vec3& aabbHalfSize, float padding, glm::vec3* intersectionAxis) {
	float returnValue = -1.0f;

	if (padding != 0.0f) {
		//Add padding
		returnValue = RayWithAabb(rayStart, rayVec, aabbPos, aabbHalfSize + glm::vec3(padding), intersectionAxis);
	}
	else {
		returnValue = RayWithAabb(rayStart, rayVec, aabbPos, aabbHalfSize, intersectionAxis);
	}

	return returnValue;
}

float Intersection::RayWithPaddedTriangle(const glm::vec3& rayStart, const glm::vec3& rayDir, const glm::vec3& triPos1, const glm::vec3& triPos2, const glm::vec3& triPos3, float padding, const bool checkBackfaces) {
	float returnValue = -1.0f;

	glm::vec3 triangleNormal = glm::normalize(glm::cross(glm::vec3(triPos1 - triPos2), glm::vec3(triPos1 - triPos3)));

	if (glm::dot(triPos1 - rayStart, triangleNormal) < 0.0f || checkBackfaces) {
		//Only check if triangle is facing ray start
		if (padding != 0.0f) {
			glm::vec3 oldV[3];
			oldV[0] = triPos1;
			oldV[1] = triPos2;
			oldV[2] = triPos3;

			glm::vec3 newV[3];

			glm::vec3 normalPadding = triangleNormal * padding;

			glm::vec3 middle = ((oldV[0] + oldV[1] + oldV[2]) / 3.0f) + normalPadding;

			for (int i = 0; i < 3; i++) {
				newV[i] = oldV[i] + normalPadding;

				float oldRayDist = glm::dot(rayDir, oldV[i] - rayStart);
				float newRayDist = glm::dot(rayDir, newV[i] - rayStart);

				glm::vec3 oldProjectionOnRayDir = rayStart + rayDir * oldRayDist;
				glm::vec3 newProjectionOnRayDir = rayStart + rayDir * newRayDist;
				float oldNormalDot = glm::dot(oldProjectionOnRayDir - oldV[i], triangleNormal);
				float newNormalDot = glm::dot(newProjectionOnRayDir - newV[i], triangleNormal);

				if ((std::signbit(oldNormalDot) != std::signbit(newNormalDot) && glm::dot(middle - newV[i], rayDir) > 0.0f) || std::signbit(oldRayDist) != std::signbit(newRayDist)) {
					glm::vec3 toRayStart = rayStart - oldV[i];
					float length = glm::min(glm::length(toRayStart), padding) - 0.001f;

					newV[i] = oldV[i] + glm::normalize(toRayStart) * length;
					//newV[i] = oldV[i];
				}
			}

			returnValue = RayWithTriangle(rayStart, rayDir, newV[0], newV[1], newV[2]);
		}
		else {
			returnValue = RayWithTriangle(rayStart, rayDir, triPos1, triPos2, triPos3);
		}
	}

	return returnValue;
}

bool Intersection::FrustumPlaneWithAabb(const glm::vec3& planeNormal, const float planeDistance, const glm::vec3* aabbCorners) {
	// Find point on positive side of plane
	for (short i = 0; i < 8; i++) {
		if ((glm::dot(aabbCorners[i], planeNormal) + planeDistance) < 0.0f) {
			return true;
		}
	}
	return false;
}

bool Intersection::FrustumWithAabb(const Frustum& frustum, const glm::vec3* aabbCorners) {
	for (int i = 0; i < 6; i++) {
		if (!FrustumPlaneWithAabb(glm::vec3(frustum.planes[i].x, frustum.planes[i].y, frustum.planes[i].z), frustum.planes[i].w, aabbCorners)) {
			//Aabb is on the wrong side of a plane - it is outside the frustum.
			return false;
		}
	}
	return true;
}

glm::vec3 Intersection::PointProjectedOnPlane(const glm::vec3& point, const glm::vec3& planeNormal, const float planeDistance) {
	const glm::vec3 pointOnPlane = planeNormal * planeDistance;
	const glm::vec3 planeToPoint = point - pointOnPlane;
	const float shortestDistanceToPlane = glm::dot(planeToPoint, planeNormal);
	return point - planeNormal * shortestDistanceToPlane;
}

bool Intersection::TriangleWithTriangleSupport(const glm::vec3 U[3], const glm::vec3 V[3], glm::vec3 outSegment[2]) {
	// Compute the plane normal for triangle U.
	glm::vec3 edge1 = U[1] - U[0];
	glm::vec3 edge2 = U[2] - U[0];
	glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

	// Test whether the edges of triangle V transversely intersect the
	// plane of triangle U.
	float d[3];
	int positive = 0, negative = 0, zero = 0;
	for (int i = 0; i < 3; ++i)
	{
		d[i] = glm::dot(normal, V[i] - U[0]);
		if (d[i] > 0.0f)
		{
			++positive;
		}
		else if (d[i] < 0.0f)
		{
			++negative;
		}
		else
		{
			++zero;
		}
	}
	// positive + negative + zero == 3

	if (positive > 0 && negative > 0)
	{
		if (positive == 2)  // and negative == 1
		{
			if (d[0] < 0.0f)
			{
				outSegment[0] = (d[1] * V[0] - d[0] * V[1]) / (d[1] - d[0]);
				outSegment[1] = (d[2] * V[0] - d[0] * V[2]) / (d[2] - d[0]);
			}
			else if (d[1] < 0.0f)
			{
				outSegment[0] = (d[0] * V[1] - d[1] * V[0]) / (d[0] - d[1]);
				outSegment[1] = (d[2] * V[1] - d[1] * V[2]) / (d[2] - d[1]);
			}
			else  // d[2] < 0.0f
			{
				outSegment[0] = (d[0] * V[2] - d[2] * V[0]) / (d[0] - d[2]);
				outSegment[1] = (d[1] * V[2] - d[2] * V[1]) / (d[1] - d[2]);
			}
		}
		else if (negative == 2)  // and positive == 1
		{
			if (d[0] > 0.0f)
			{
				outSegment[0] = (d[1] * V[0] - d[0] * V[1]) / (d[1] - d[0]);
				outSegment[1] = (d[2] * V[0] - d[0] * V[2]) / (d[2] - d[0]);
			}
			else if (d[1] > 0.0f)
			{
				outSegment[0] = (d[0] * V[1] - d[1] * V[0]) / (d[0] - d[1]);
				outSegment[1] = (d[2] * V[1] - d[1] * V[2]) / (d[2] - d[1]);
			}
			else  // d[2] > 0.0f
			{
				outSegment[0] = (d[0] * V[2] - d[2] * V[0]) / (d[0] - d[2]);
				outSegment[1] = (d[1] * V[2] - d[2] * V[1]) / (d[1] - d[2]);
			}
		}
		else  // positive == 1, negative == 1, zero == 1
		{
			if (d[0] == 0.0f)
			{
				outSegment[0] = V[0];
				outSegment[1] = (d[2] * V[1] - d[1] * V[2]) / (d[2] - d[1]);
			}
			else if (d[1] == 0.0f)
			{
				outSegment[0] = V[1];
				outSegment[1] = (d[0] * V[2] - d[2] * V[0]) / (d[0] - d[2]);
			}
			else  // d[2] == 0.0f
			{
				outSegment[0] = V[2];
				outSegment[1] = (d[1] * V[0] - d[0] * V[1]) / (d[1] - d[0]);
			}
		}
		return true;
	}

	// Triangle V does not transversely intersect triangle U, although it is
	// possible a vertex or edge of V is just touching U.  In this case, we
	// do not call this an intersection.
	return false;
}

void Intersection::Barycentric(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float& u, float& v, float& w) {
	const glm::vec3 v0 = b - a;
	const glm::vec3 v1 = c - a;
	const glm::vec3 v2 = p - a;

	// Determine barycentric coordinates
	const float d00 = glm::dot(v0, v0);
	const float d01 = glm::dot(v0, v1);
	const float d11 = glm::dot(v1, v1);
	const float d20 = glm::dot(v2, v0);
	const float d21 = glm::dot(v2, v1);
	const float denom = d00 * d11 - d01 * d01;
	const float divDenom = 1.0f / denom;
	u = (d11 * d20 - d01 * d21) * divDenom;
	v = (d00 * d21 - d01 * d20) * divDenom;
	w = 1.0f - u - v;
}

bool Intersection::OnTriangle(const float u, const float v, const float w) {
	return ((0.0f <= v && v <= 1.0f) && (0.0f <= w && w <= 1.0f) && (0.0f <= u && u <= 1.0f));
}

bool Intersection::SATTest(const glm::vec3& testAxis, const glm::vec3& triPos1, const glm::vec3& triPos2, const glm::vec3& triPos3, const glm::vec3& aabbSize, glm::vec3* intersectionAxis, float* depth) {
	glm::vec3 p = glm::vec3(glm::dot(testAxis, triPos1), glm::dot(testAxis, triPos2), glm::dot(testAxis, triPos3));
	float r = aabbSize.x * glm::abs(testAxis.x) + aabbSize.y * glm::abs(testAxis.y) + aabbSize.z * glm::abs(testAxis.z);

	float tempDepth = glm::min(r - glm::min(p.x, glm::min(p.y, p.z)), glm::max(p.x, glm::max(p.y, p.z)) + r);

	if (tempDepth < 0.0f) {
		return false;
	}
	else {
		//Save depth along axis if lower than previous
		if (tempDepth < *depth) {
			*depth = tempDepth;
			*intersectionAxis = testAxis;
		}
	}
	return true;
}
