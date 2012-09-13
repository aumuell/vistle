#ifndef OBJECT_H
#define OBJECT_H

#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace vistle {

typedef boost::interprocess::managed_shared_memory::handle_t shm_handle_t;

namespace message {
   class MessageQueue;
}

class Object;

class Shm {

 public:
   static Shm & instance(const int moduleID = -1, const int rank = -1,
                         message::MessageQueue * messageQueue = NULL);
   ~Shm();

   boost::interprocess::managed_shared_memory & getShm();
   std::string createObjectID();

   void publish(const shm_handle_t & handle);
   Object * getObjectFromHandle(const shm_handle_t & handle);

 private:
   Shm(const int moduleID, const int rank, const size_t size,
       message::MessageQueue *messageQueue);

   const int m_moduleID;
   const int m_rank;
   int m_objectID;
   static Shm *s_singleton;
   boost::interprocess::managed_shared_memory *m_shm;
   message::MessageQueue *m_messageQueue;
};

template<typename T>
struct shm {
   typedef boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> allocator;
   typedef boost::interprocess::vector<T, allocator> vector;
   typedef boost::interprocess::offset_ptr<vector> ptr;
};

class Object {

 public:
   enum Type {
      UNKNOWN           = -1,
      VECFLOAT          =  0,
      VECINT            =  1,
      VECCHAR           =  2,
      VEC3FLOAT         =  3,
      VEC3INT           =  4,
      VEC3CHAR          =  5,
      TRIANGLES         =  6,
      LINES             =  7,
      POLYGONS          =  8,
      UNSTRUCTUREDGRID  =  9,
      SET               = 10,
      GEOMETRY          = 11,
      TEXTURE1D         = 12,
   };

   Object(const Type id, const std::string & name,
          const int block, const int timestep);
   virtual ~Object();

   Type getType() const;
   std::string getName() const;

   int getBlock() const;
   int getTimestep() const;

   void setBlock(const int block);
   void setTimestep(const int timestep);

 protected:
   const Type m_type;
 private:
   char m_name[32];

   int m_block;
   int m_timestep;
};


template <class T>
class Vec: public Object {

 public:
   static Vec<T> * create(const size_t size = 0,
                          const int block = -1, const int timestep = -1) {

      const std::string name = Shm::instance().createObjectID();
      Vec<T> *t = static_cast<Vec<T> *>
         (Shm::instance().getShm().construct<Vec<T> >(name.c_str())[1](size, name, block, timestep));
      /*
      shm_handle_t handle =
         Shm::instance().getShm().get_handle_from_address(t);
      Shm::instance().publish(handle);
      */
      return t;
   }

 Vec(const size_t size, const std::string & name,
     const int block, const int timestep)
    : Object(s_type, name, block, timestep) {

      const typename shm<T>::allocator
         alloc_inst(Shm::instance().getShm().get_segment_manager());

      x = Shm::instance().getShm().construct<typename shm<T>::vector> (Shm::instance().createObjectID().c_str())(size, T(), alloc_inst);
   }

   size_t getSize() const {
      return x->size();
   }

   void setSize(const size_t size) {
      x->resize(size);
   }

   typename shm<T>::ptr x;

 private:
   static const Object::Type s_type;
};


template <class T>
class Vec3: public Object {

 public:
   static Vec3<T> * create(const size_t size = 0,
                           const int block = -1, const int timestep = -1) {

      const std::string name = Shm::instance().createObjectID();
      Vec3<T> *t = static_cast<Vec3<T> *>
         (Shm::instance().getShm().construct<Vec3<T> >(name.c_str())[1](size, name, block, timestep));
      /*
      shm_handle_t handle =
         Shm::instance().getShm().get_handle_from_address(t);
      Shm::instance().publish(handle);
      */
      return t;
   }

   Vec3(const size_t size, const std::string & name,
        const int block, const int timestep)
      : Object(s_type, name, block, timestep) {

      const typename shm<T>::allocator
         alloc_inst(Shm::instance().getShm().get_segment_manager());

      x = Shm::instance().getShm().construct<typename shm<T>::vector> (Shm::instance().createObjectID().c_str())(size, T(), alloc_inst);
      y = Shm::instance().getShm().construct<typename shm<T>::vector> (Shm::instance().createObjectID().c_str())(size, T(), alloc_inst);
      z = Shm::instance().getShm().construct<typename shm<T>::vector> (Shm::instance().createObjectID().c_str())(size, T(), alloc_inst);
   }

   size_t getSize() const {
      return x->size();
   }

   void setSize(const size_t size) {
      x->resize(size);
      y->resize(size);
      z->resize(size);
   }

   typename shm<T>::ptr x, y, z;

 private:
   static const Object::Type s_type;
};

class Triangles: public Object {

 public:
   static Triangles * create(const size_t numCorners = 0,
                             const size_t numVertices = 0,
                             const int block = -1, const int timestep = -1);

   Triangles(const size_t numCorners, const size_t numVertices,
             const std::string & name,
             const int block, const int timestep);

   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::ptr cl;
   shm<float>::ptr x, y, z;
 private:
};

class Lines: public Object {

 public:
   static Lines * create(const size_t numElements = 0,
                         const size_t numCorners = 0,
                         const size_t numVertices = 0,
                         const int block = -1, const int timestep = -1);

   Lines(const size_t numElements, const size_t numCorners,
         const size_t numVertices, const std::string & name,
         const int block, const int timestep);

   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::ptr el, cl;
   shm<float>::ptr x, y, z;

 private:
};

class Polygons: public Object {

 public:
   static Polygons * create(const size_t numElements = 0,
                            const size_t numCorners = 0,
                            const size_t numVertices = 0,
                            const int block = -1, const int timestep = -1);
   Polygons(const size_t numElements, const size_t numCorners,
            const size_t numVertices, const std::string & name,
            const int block, const int timestep);

   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::ptr el, cl;
   shm<float>::ptr x, y, z;

 private:
};


class UnstructuredGrid: public Object {

 public:
   enum Type {
      NONE        =  0,
      BAR         =  1,
      TRIANGLE    =  2,
      QUAD        =  3,
      TETRAHEDRON =  4,
      PYRAMID     =  5,
      PRISM       =  6,
      HEXAHEDRON  =  7,
      POINT       = 10,
      POLYHEDRON  = 11
   };

   static UnstructuredGrid * create(const size_t numElements = 0,
                                    const size_t numCorners = 0,
                                    const size_t numVertices = 0,
                                    const int block = -1,
                                    const int timestep = -1);

   UnstructuredGrid(const size_t numElements, const size_t numCorners,
                    const size_t numVertices, const std::string & name,
                    const int block, const int timestep);

   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<char>::ptr tl;
   shm<size_t>::ptr el, cl;
   shm<float>::ptr x, y, z;

 private:

};

class Set: public Object {

 public:
   static Set * create(const size_t numElements = 0,
                       const int block = -1, const int timestep = -1);

   Set(const size_t numElements, const std::string & name,
       const int block, const int timestep);

   size_t getNumElements() const;
   Object * getElement(const size_t index) const;

   shm<boost::interprocess::offset_ptr<Object> >::ptr elements;
};

class Geometry: public Object {

 public:
   static Geometry * create(const int block = -1, const int timestep = -1);

   Geometry(const std::string & name,
            const int block, const int timestep);

   boost::interprocess::offset_ptr<Object> geometry;
   boost::interprocess::offset_ptr<Object> colors;
   boost::interprocess::offset_ptr<Object> normals;
   boost::interprocess::offset_ptr<Object> texture;

 private:

};

class Texture1D: public Object {

 public:
   static Texture1D * create(const size_t width = 0,
                             const float min = 0, const float max = 0,
                             const int block = -1, const int timestep = -1);

   Texture1D(const std::string & name, const size_t size,
             const float min, const float max,
             const int block, const int timestep);

   shm<unsigned char>::ptr pixels;
   shm<float>::ptr coords;

   size_t getNumElements() const;
   size_t getWidth() const;

   float min;
   float max;
};

} // namespace vistle
#endif
