#ifndef SPHERES_IMPL_H
#define SPHERES_IMPL_H

namespace vistle {

template<class Archive>
void Spheres::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base_coords_with_radius", boost::serialization::base_object<Base::Data>(*this));
}

} // namespace vistle

#endif
