// EngineBuildingBlocks/Math/GLM.h

#ifndef _ENGINEBUILDINGBLOCKS_GLM_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_GLM_H_INCLUDED_

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/dual_quaternion.hpp>

namespace EngineBuildingBlocks
{
	template <typename T>
	inline T SafeNormalize(const T& v)
	{
		auto length = glm::length(v);
		if (length > 0.0f) return v / length;
		return v;
	}

	template <>
	inline glm::dualquat SafeNormalize(const glm::dualquat& dq)
	{
		auto length = glm::length(dq.real);
		if (length > 0.0f) return dq / length;
		return dq;
	}
}

#endif
