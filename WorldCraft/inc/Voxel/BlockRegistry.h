#pragma once

#include <Voxel/BlockTypes.h>
#include <cstdint>

namespace Voxel
{

	// Runtime properties for a single block type.
	struct BlockProperties
	{
		bool isSolid       = false; // Solid blocks cull adjacent faces
		bool isTransparent = false; // Transparent blocks need a separate draw pass
		const char* name   = "unknown";

		// Texture layer indices (into the GL_TEXTURE_2D_ARRAY) per face direction.
		// Indexed by static_cast<uint8_t>(FaceDir).
		int texLayer[static_cast<size_t>(FaceDir::COUNT)] = {};
	};

	// Singleton registry — look up properties by BlockID.
	class BlockRegistry
	{
	public:
		// Access the single shared instance.
		static const BlockRegistry& get();

		// Return properties for the given block ID.
		// Falls back to Air properties for out-of-range values.
		const BlockProperties& propertiesOf(BlockID id) const;

		// Convenience: returns true when id is considered solid.
		bool isSolid(BlockID id) const;

		// Return the texture array layer index for a given block face.
		int texLayer(BlockID id, FaceDir face) const;

	private:
		BlockRegistry();                      // fills the table on construction
		BlockProperties m_table[static_cast<size_t>(BlockID::COUNT)];
	};

} // namespace Voxel
