#ifndef ISOSURFACE_H
#define ISOSURFACE_H

#include <module/module.h>
#include <core/vec.h>
#include <core/unstr.h>
#include "IsoDataFunctor.h"

class IsoSurface: public vistle::Module {

 public:
   IsoSurface(const std::string &shmname, const std::string &name, int moduleID);
   ~IsoSurface();

 private:
   std::pair<vistle::Object::ptr, vistle::Object::ptr> generateIsoSurface(
                                       vistle::Object::const_ptr grid,
                                       vistle::Object::const_ptr data,
                                       vistle::Object::const_ptr mapdata,
                                       const vistle::Scalar isoValue,
                                       int processorType
                                        );

   std::vector<vistle::UnstructuredGrid::const_ptr> m_grids;
   std::vector<vistle::Vec<vistle::Scalar>::const_ptr> m_datas;
   std::vector<vistle::DataBase::const_ptr> m_mapdatas;

   bool work(vistle::UnstructuredGrid::const_ptr grid,
             vistle::Vec<vistle::Scalar>::const_ptr data,
             vistle::DataBase::const_ptr mapdata);
   virtual bool compute() override;
   virtual bool prepare() override;
   virtual bool reduce(int timestep) override;
   bool changeParameter(const vistle::Parameter *param) override;



   vistle::FloatParameter *m_isovalue;
   vistle::VectorParameter *m_isopoint;
   vistle::IntParameter *m_pointOrValue;
   vistle::IntParameter *m_processortype;
   vistle::Port *m_mapDataIn, *m_dataOut;

   vistle::Scalar m_min, m_max;
   vistle::Float m_paraMin, m_paraMax;

   IsoController isocontrol;
};

#endif
