﻿//
//This code is used for both IsoCut and IsoSurface!
//

#include <iostream>
#include <cmath>
#include <ctime>
#include <boost/mpi/collectives.hpp>
#include <core/message.h>
#include <core/object.h>
#include <core/unstr.h>
#include <core/uniformgrid.h>
#include <core/rectilineargrid.h>
#include <core/structuredgrid.h>
#include <core/vec.h>

#ifdef CUTTINGSURFACE
#define IsoSurface CuttingSurface
#endif


#include "IsoSurface.h"
#include "Leveller.h"

#ifdef CUTTINGSURFACE
MODULE_MAIN(CuttingSurface)
#else
MODULE_MAIN(IsoSurface)
#endif

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(PointOrValue,
                                    (Point)
                                    (Value)
)

IsoSurface::IsoSurface(const std::string &name, int moduleID, mpi::communicator comm)
   : Module(
#ifdef CUTTINGSURFACE
"cut through grids"
#else
"extract isosurfaces"
#endif
, name, moduleID, comm)
, isocontrol(this) {

   isocontrol.init();

   setDefaultCacheMode(ObjectCache::CacheDeleteLate);
#ifdef CUTTINGSURFACE
   m_mapDataIn = createInputPort("data_in");
#else
   m_isovalue = addFloatParameter("isovalue", "isovalue", 0.0);
   m_isopoint = addVectorParameter("isopoint", "isopoint", ParamVector(0.0, 0.0, 0.0));
   setReducePolicy(message::ReducePolicy::Locally);
   m_pointOrValue = addIntParameter("point_or_value", "point or value interaction", Value, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_pointOrValue, PointOrValue);

   createInputPort("data_in");
   m_mapDataIn = createInputPort("mapdata_in");
#endif    
   m_dataOut = createOutputPort("data_out");

   m_processortype = addIntParameter("processortype", "processortype", 0, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_processortype, ThrustBackend);

   m_computeNormals = addIntParameter("compute_normals", "compute normals (structured grids only)", 1, Parameter::Boolean);

   m_paraMin = m_paraMax = 0.f;
}

IsoSurface::~IsoSurface() = default;

bool IsoSurface::changeParameter(const Parameter* param) {

    bool ok = isocontrol.changeParameter(param);

#ifndef CUTTINGSURFACE
    if (param == m_pointOrValue) {
        if (m_pointOrValue->getValue() == Point)
            setReducePolicy(message::ReducePolicy::PerTimestepZeroFirst);
        else
            setReducePolicy(message::ReducePolicy::Locally);
    }
#endif

   return Module::changeParameter(param) && ok;
}

bool IsoSurface::prepare() {

   m_grids.clear();
   m_datas.clear();
   m_mapdatas.clear();

   m_min = std::numeric_limits<Scalar>::max() ;
   m_max = -std::numeric_limits<Scalar>::max();

   m_performedPointSearch = false;
   m_foundPoint = false;

   return Module::prepare();
}

bool IsoSurface::reduce(int timestep) {

#ifndef CUTTINGSURFACE
   if (rank() == 0)
       std::cerr << "IsoSurface::reduce(" << timestep << ")" << std::endl;
   if (timestep <=0 && m_pointOrValue->getValue() == Point && !m_performedPointSearch) {
       m_performedPointSearch = true;
       Scalar value = m_isovalue->getValue();
       int found = 0;
       Vector point = m_isopoint->getValue();
       int nb = 0;
       for (size_t i=0; i<m_grids.size(); ++i) {
           int t = m_grids[i]->getTimestep();
           if (t < 0)
               t = m_datas[i]->getTimestep();
           if (t==0 || t==-1) {
               ++nb;
               auto gi = m_grids[i]->getInterface<GridInterface>();
               Index cell = gi->findCell(point);
               if (cell != InvalidIndex) {
                   ++found;
                   auto interpol = gi->getInterpolator(cell, point);
                   value = interpol(m_datas[i]->x());
               }
           }
       }
       std::cerr << "found " << nb << " candidate blocks" << std::endl;
       int numFound = boost::mpi::all_reduce(comm(), found, std::plus<int>());
       m_foundPoint = numFound>0;
       if (m_rank == 0) {
           if (numFound == 0)
               sendInfo("iso-value point out of domain");
           else if (numFound > 1)
               sendWarning("found isopoint in %d blocks", numFound);
       }
       int valRank = found ? m_rank : m_size;
       valRank = boost::mpi::all_reduce(comm(), valRank, boost::mpi::minimum<int>());
       if (valRank < m_size) {
           boost::mpi::broadcast(comm(), value, valRank);
           setParameter(m_isovalue, (Float)value);
           setParameter(m_pointOrValue, (Integer)Point);
       }
   }

   if (m_foundPoint) {
       auto task = std::make_shared<PortTask>(const_cast<IsoSurface *>(this));
       for (size_t i=0; i<m_grids.size(); ++i) {
           int t = m_grids[i] ? m_grids[i]->getTimestep() : -1;
           if (m_datas[i])
               t = std::max(t, m_datas[i]->getTimestep());
           if (m_mapdatas[i])
               t = std::max(t, m_mapdatas[i]->getTimestep());
           if (t == timestep)
               work(task, m_grids[i], m_datas[i], m_mapdatas[i]);
       }
   }

   if (timestep == -1) {
       Scalar min, max;
       boost::mpi::all_reduce(comm(),
                              m_min, min, boost::mpi::minimum<Scalar>());
       boost::mpi::all_reduce(comm(),
                              m_max, max, boost::mpi::maximum<Scalar>());

       if (max >= min) {
           if (m_paraMin != (Float)min || m_paraMax != (Float)max)
               setParameterRange(m_isovalue, (Float)min, (Float)max);

           m_paraMax = max;
           m_paraMin = min;
       }
   }
#endif

   return Module::reduce(timestep);
}

bool IsoSurface::work(std::shared_ptr<PortTask> task,
             vistle::Object::const_ptr grid,
             vistle::Vec<vistle::Scalar>::const_ptr dataS,
             vistle::DataBase::const_ptr mapdata) const {

   const int processorType = getIntParameter("processortype");
#ifdef CUTTINGSURFACE
   const Scalar isoValue = 0.0;
#else
   const Scalar isoValue = getFloatParameter("isovalue");
#endif

   Leveller l(isocontrol, grid, isoValue, processorType);
   l.setComputeNormals(m_computeNormals->getValue());

#ifndef CUTTINGSURFACE
   l.setIsoData(dataS);
#endif
   if(mapdata){
      l.addMappedData(mapdata);
   };
   l.process();

#ifndef CUTTINGSURFACE
   auto minmax = dataS->getMinMax();
   {
       std::lock_guard<std::mutex> guard(m_mutex);
       if (minmax.first[0] < m_min)
           m_min = minmax.first[0];
       if (minmax.second[0] > m_max)
           m_max = minmax.second[0];
   }
#endif

   Object::ptr result = l.result();
   DataBase::ptr mapresult = l.mapresult();
   if (result && !result->isEmpty()) {
#ifndef CUTTINGSURFACE
      result->copyAttributes(dataS);
#endif
      result->updateInternals();
      result->copyAttributes(grid, false);
      result->setTransform(grid->getTransform());
      if (result->getTimestep() < 0) {
          result->setTimestep(grid->getTimestep());
          result->setNumTimesteps(grid->getNumTimesteps());
      }
      if (result->getBlock() < 0) {
          result->setBlock(grid->getBlock());
          result->setNumBlocks(grid->getNumBlocks());
      }
      if (mapdata && mapresult) {
         mapresult->updateInternals();
         mapresult->copyAttributes(mapdata);
         mapresult->setGrid(result);
         task->addObject(m_dataOut, mapresult);
      }
#ifndef CUTTINGSURFACE
      else {
          task->addObject(m_dataOut, result);
      }
#endif
   }
   return true;
}

#if 0
bool IsoSurface::compute() {

#ifdef CUTTINGSURFACE
   auto mapdata = expect<DataBase>(m_mapDataIn);
   if (!mapdata)
       return true;
   auto grid = mapdata->grid();
#else
   auto mapdata = accept<DataBase>(m_mapDataIn);
   auto dataS = expect<Vec<Scalar>>("data_in");
   if (!dataS)
      return true;
   if (dataS->guessMapping() != DataBase::Vertex) {
      sendError("need per-vertex mapping on data_in");
      return true;
   }
   auto grid = dataS->grid();
#endif
   auto uni = UniformGrid::as(grid);
   auto rect = RectilinearGrid::as(grid);
   auto str = StructuredGrid::as(grid);
   auto unstr = UnstructuredGrid::as(grid);
   if (!uni && !rect && !str && !unstr) {
       if (grid)
           sendError("grid required on input data: invalid type");
       else
           sendError("grid required on input data: none present");
       return true;
   }

#ifdef CUTTINGSURFACE
    return work(grid, nullptr, mapdata);
#else
    if (m_pointOrValue->getValue() == Value) {
        return work(grid, dataS, mapdata);
    } else {
        int t = -1;
        //unstr->getCelltree();
        if (grid)
            t = grid->getTimestep();
        m_grids.push_back(grid);
        m_datas.push_back(dataS);
        if (t < 0)
            t = dataS->getTimestep();
        m_mapdatas.push_back(mapdata);
        //std::cerr << "compute with t=" << t << std::endl;
        return true;
    }
#endif
}
#endif


bool IsoSurface::compute(std::shared_ptr<PortTask> task) const {

#ifdef CUTTINGSURFACE
    auto mapdata = task->expect<DataBase>(m_mapDataIn);
   if (!mapdata)
       return true;
   auto grid = mapdata->grid();
#else
    auto mapdata = task->accept<DataBase>(m_mapDataIn);
    auto dataS = task->expect<Vec<Scalar>>("data_in");
   if (!dataS)
      return true;
   if (dataS->guessMapping() != DataBase::Vertex) {
       sendError("need per-vertex mapping on data_in");
      return true;
   }
   auto grid = dataS->grid();
#endif
   auto uni = UniformGrid::as(grid);
   auto rect = RectilinearGrid::as(grid);
   auto str = StructuredGrid::as(grid);
   auto unstr = UnstructuredGrid::as(grid);
   if (!uni && !rect && !str && !unstr) {
       if (grid)
           sendError("grid required on input data: invalid type");
       else
           sendError("grid required on input data: none present");
       return true;
   }

#ifdef CUTTINGSURFACE
    return work(task, grid, nullptr, mapdata);
#else
    if (m_pointOrValue->getValue() == Value) {
        return work(task, grid, dataS, mapdata);
    } else {
        int t = -1;
        //unstr->getCelltree();
        if (grid)
            t = grid->getTimestep();
        std::lock_guard<std::mutex> guard(m_mutex);
        m_grids.push_back(grid);
        m_datas.push_back(dataS);
        if (t < 0)
            t = dataS->getTimestep();
        m_mapdatas.push_back(mapdata);
        //std::cerr << "compute with t=" << t << std::endl;
        return true;
    }
#endif
}
