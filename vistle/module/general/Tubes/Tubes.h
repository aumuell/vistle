#ifndef TOTUBES_H
#define TOTUBES_H

#include <module/module.h>

class ToTubes: public vistle::Module {

 public:
   ToTubes(const std::string &shmname, const std::string &name, int moduleID);
   ~ToTubes();

 private:
   virtual bool compute();

   vistle::FloatParameter *m_radius;
   vistle::IntParameter *m_mapMode;
   vistle::VectorParameter *m_range;
   vistle::IntParameter *m_startStyle, *m_jointStyle, *m_endStyle;
};

#endif
