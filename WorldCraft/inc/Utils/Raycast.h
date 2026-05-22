#pragma once

#include <Voxel/BlockTypes.h>
#include <glm/glm.hpp>
#include <optional>

namespace Utils
{

	// Result of a voxel raycast, containing the hit block position and face.
	struct RaycastHit
	{
		glm::ivec3       blockPos;    // Integer block coordinates of the hit block
		Voxel::FaceDir   face;        // Which face of the block was hit
		glm::vec3        hitPoint;    // Exact world-space hit position
		float            distance;    // Distance from ray origin to hit point
	};

	// ---------------------------------------------------------------------------
	// Raycast through a voxel world using DDA (Digital Differential Analyzer).
	//
	// Traverses voxels along a ray from origin in direction dir, up to maxDist.
	// Calls the provided callback for each voxel; if callback returns true,
	// the voxel is considered solid and the raycast stops, returning a hit.
	//
	// The callback signature is: bool callback(int x, int y, int z)
	//   Returns true if the voxel at (x,y,z) is solid (stops the raycast).
	//
	// Returns std::nullopt if no solid voxel is hit within maxDist.
	// ---------------------------------------------------------------------------
	template<typename SolidCheckFunc>
	std::optional<RaycastHit> raycastVoxel(
		const glm::vec3& origin,
		const glm::vec3& direction,
		float maxDist,
		SolidCheckFunc&& isSolid)
	{
		// Normalize direction
		glm::vec3 dir = glm::normalize(direction);

		// Current voxel coordinates
		glm::ivec3 voxel = glm::floor(origin);

		// Step direction (+1 or -1) for each axis
		glm::ivec3 step = glm::ivec3(
			dir.x >= 0.0f ? 1 : -1,
			dir.y >= 0.0f ? 1 : -1,
			dir.z >= 0.0f ? 1 : -1
		);

		// tMax: distance along ray to next voxel boundary on each axis
		// tDelta: how far along ray to move to cross one voxel on each axis
		glm::vec3 tMax, tDelta;

		// Initialize tMax and tDelta for each axis
		for (int i = 0; i < 3; ++i)
		{
			if (std::abs(dir[i]) < 1e-8f)
			{
				// Ray is parallel to this axis — never crosses a boundary
				tMax[i]   = std::numeric_limits<float>::infinity();
				tDelta[i] = std::numeric_limits<float>::infinity();
			}
			else
			{
				// Calculate distance to next voxel boundary
				float invDir = 1.0f / dir[i];
				float voxelBoundary = static_cast<float>(voxel[i] + (step[i] > 0 ? 1 : 0));
				tMax[i]   = (voxelBoundary - origin[i]) * invDir;
				tDelta[i] = static_cast<float>(step[i]) * invDir;
			}
		}

		// Track which face was crossed to enter current voxel
		Voxel::FaceDir hitFace = Voxel::FaceDir::PosX; // default (will be overwritten on first hit)
		float          tCurrent = 0.0f;

		// DDA traversal
		constexpr int maxSteps = 256; // Safety limit to prevent infinite loops
		for (int iter = 0; iter < maxSteps; ++iter)
		{
			// Check if current voxel is solid
			if (isSolid(voxel.x, voxel.y, voxel.z))
			{
				// Hit! Calculate exact hit point and return
				glm::vec3 hitPoint = origin + dir * tCurrent;
				return RaycastHit{
					voxel,
					hitFace,
					hitPoint,
					tCurrent
				};
			}

			// Check if we've exceeded max distance
			if (tCurrent > maxDist)
				break;

			// Step to next voxel along the axis with smallest tMax
			if (tMax.x < tMax.y)
			{
				if (tMax.x < tMax.z)
				{
					// Step along X
					tCurrent = tMax.x;
					tMax.x += tDelta.x;
					voxel.x += step.x;
					hitFace = (step.x > 0) ? Voxel::FaceDir::NegX : Voxel::FaceDir::PosX;
				}
				else
				{
					// Step along Z
					tCurrent = tMax.z;
					tMax.z += tDelta.z;
					voxel.z += step.z;
					hitFace = (step.z > 0) ? Voxel::FaceDir::NegZ : Voxel::FaceDir::PosZ;
				}
			}
			else
			{
				if (tMax.y < tMax.z)
				{
					// Step along Y
					tCurrent = tMax.y;
					tMax.y += tDelta.y;
					voxel.y += step.y;
					hitFace = (step.y > 0) ? Voxel::FaceDir::NegY : Voxel::FaceDir::PosY;
				}
				else
				{
					// Step along Z
					tCurrent = tMax.z;
					tMax.z += tDelta.z;
					voxel.z += step.z;
					hitFace = (step.z > 0) ? Voxel::FaceDir::NegZ : Voxel::FaceDir::PosZ;
				}
			}
		}

		// No hit within max distance
		return std::nullopt;
	}

} // namespace Utils
