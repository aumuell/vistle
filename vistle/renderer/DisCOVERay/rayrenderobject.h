#ifndef RAY_RENDEROBJECT_H
#define RAY_RENDEROBJECT_H

#include <vector>
#include <memory>

#include <core/vector.h>
#include <core/object.h>
#include <core/normals.h>
#include <core/texture1d.h>

#include <renderer/renderobject.h>

#include "renderobjectdata_ispc.h"
#include "render_ispc.h"

#include "common.h"

#ifdef USE_STREAM
static const RTCAlgorithmFlags intersections = RTC_INTERSECT1|RTC_INTERSECT4|RTC_INTERSECT8|RTC_INTERSECT16|RTC_INTERSECT_STREAM;
#else
static const RTCAlgorithmFlags intersections = RTC_INTERSECT1|RTC_INTERSECT4|RTC_INTERSECT8|RTC_INTERSECT16;
#endif

struct RayRenderObject: public vistle::RenderObject {

   static float pointSize;

   RayRenderObject(RTCDevice device, int senderId, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr texture);

   ~RayRenderObject();

   std::unique_ptr<ispc::RenderObjectData> data;
};
#endif
