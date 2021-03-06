#ifndef VECTORFIELD_H
#define VECTORFIELD_H

#include <module/module.h>

class VectorField: public vistle::Module {

 public:
   VectorField(const std::string &shmname, const std::string &name, int moduleID);
   ~VectorField();

 private:
   virtual bool compute();

   vistle::FloatParameter *m_scale;
   vistle::VectorParameter *m_range;
   vistle::IntParameter *m_attachmentPoint;
};

#endif
