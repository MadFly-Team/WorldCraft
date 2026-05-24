#pragma once

#include <functional>
#include <utility>

namespace std
{
	// Hash specialization for std::pair<int, int> to enable use in unordered containers
	template<>
	struct hash<std::pair<int, int>>
	{
		size_t operator()(const std::pair<int, int>& p) const noexcept
		{
			// Combine the two hash values using a standard technique
			return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
		}
	};
}
