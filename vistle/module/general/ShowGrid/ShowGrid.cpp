#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/lines.h>
#include <core/unstr.h>
#include <core/structuredgridbase.h>
#include <core/structuredgrid.h>

#include "ShowGrid.h"

MODULE_MAIN(ShowUSG)

using namespace vistle;

ShowUSG::ShowUSG(const std::string &shmname, const std::string &name, int moduleID)
   : Module("show outlines of grid cells", shmname, name, moduleID) {

   setDefaultCacheMode(ObjectCache::CacheDeleteLate);

   createInputPort("grid_in");
   createOutputPort("grid_out");

   addIntParameter("normalcells", "Show normal (non ghost) cells", 1, Parameter::Boolean);
   addIntParameter("ghostcells", "Show ghost cells", 0, Parameter::Boolean);
   addIntParameter("convex", "Show convex cells", 1, Parameter::Boolean);
   addIntParameter("nonconvex", "Show non-convex cells", 1, Parameter::Boolean);

   addIntParameter("tetrahedron", "Show tetrahedron", 1, Parameter::Boolean);
   addIntParameter("pyramid", "Show pyramid", 1, Parameter::Boolean);
   addIntParameter("prism", "Show prism", 1, Parameter::Boolean);
   addIntParameter("hexahedron", "Show hexahedron", 1, Parameter::Boolean);
   addIntParameter("polyhedron", "Show polyhedron", 1, Parameter::Boolean);
   m_CellNrMin = addIntParameter("Show Cells from Cell Nr. :", "Show Cell Nr.", -1);
   m_CellNrMax = addIntParameter("Show Cells to Cell Nr. :", "Show Cell Nr.", -1);

}

ShowUSG::~ShowUSG() {

}

bool ShowUSG::compute() {

   const bool shownor = getIntParameter("normalcells");
   const bool showgho = getIntParameter("ghostcells");
   const bool showconv = getIntParameter("convex");
   const bool shownonconv = getIntParameter("nonconvex");

   const bool showtet = getIntParameter("tetrahedron");
   const bool showpyr = getIntParameter("pyramid");
   const bool showpri = getIntParameter("prism");
   const bool showhex = getIntParameter("hexahedron");
   const bool showpol = getIntParameter("polyhedron");
   const Integer cellnrmin = getIntParameter("Show Cells from Cell Nr. :");
   const Integer cellnrmax = getIntParameter("Show Cells to Cell Nr. :");


   Object::const_ptr grid;
   auto obj = expect<Object>("grid_in");
   if (auto db = DataBase::as(obj)) {
       grid = db->grid();
   }
   if (!grid)
       grid = obj;
   if (!grid) {
       sendError("did not receive an input object");
       return true;
   }

   vistle::Lines::ptr out(new vistle::Lines(Object::Initialized));
   auto &ocl = out->cl();
   auto &oel = out->el();

   if (auto unstr = UnstructuredGrid::as(grid)) {
      const Index *icl = &unstr->cl()[0];

      Index begin = 0, end = unstr->getNumElements();
      if (cellnrmin >= 0)
          begin = std::max(cellnrmin, (Integer)begin);
      if (cellnrmax >= 0)
          end = std::min(cellnrmax+1, (Integer)end);

      for (Index index = begin; index < end; ++index) {
          auto type=unstr->tl()[index];
          const bool ghost = type & UnstructuredGrid::GHOST_BIT;
          const bool conv = type & UnstructuredGrid::CONVEX_BIT;

          const bool show = ((showgho && ghost) || (shownor && !ghost))
                  && ((showconv && conv) || (shownonconv && !conv));
          if (!show)
              continue;
          type &= vistle::UnstructuredGrid::TYPE_MASK;
          switch(type) {
          case UnstructuredGrid::TETRAHEDRON:
              if (!showtet) { continue; } break;
          case UnstructuredGrid::PYRAMID:
              if (!showpyr) { continue; } break;
          case UnstructuredGrid::PRISM:
              if (!showpri) { continue; } break;
          case UnstructuredGrid::HEXAHEDRON:
              if (!showhex) { continue; } break;
          case UnstructuredGrid::POLYHEDRON:
              if (!showpol) { continue; } break;
          }

          const Index begin=unstr->el()[index], end=unstr->el()[index+1];
          switch(type) {
          case UnstructuredGrid::TETRAHEDRON:
          case UnstructuredGrid::PYRAMID:
          case UnstructuredGrid::PRISM:
          case UnstructuredGrid::HEXAHEDRON: {
              const auto numFaces = UnstructuredGrid::NumFaces[type];
              for (int f=0; f<numFaces; ++f) {
                  const int nCorners = UnstructuredGrid::FaceSizes[type][f];
                  for (int i=0; i<=nCorners; ++i) {
                      const Index v = icl[begin+UnstructuredGrid::FaceVertices[type][f][i%nCorners]];
                      ocl.push_back(v);
                  }
                  oel.push_back(ocl.size());
              }
              break;
          }
          case UnstructuredGrid::POLYHEDRON: {
              for (Index i = begin; i < end; i += icl[i]) {
                  const Index numFaceVert = icl[i];
                  for (Index k=i+1; k<i+numFaceVert+1; ++k)
                      ocl.push_back(icl[k]);
                  ocl.push_back(icl[i+1]);
                  oel.push_back(ocl.size());
              }
              break;
          }
          }
      }
      out->d()->x[0] = unstr->d()->x[0];
      out->d()->x[1] = unstr->d()->x[1];
      out->d()->x[2] = unstr->d()->x[2];
   } else if (auto str = StructuredGridBase::as(grid)) {
       const Index dims[3] = { str->getNumDivisions(0), str->getNumDivisions(1), str->getNumDivisions(2) };
       Index begin = 0, end = str->getNumElements();
       if (cellnrmin >= 0)
           begin = std::max(cellnrmin, (Integer)begin);
       if (cellnrmax >= 0)
           end = std::min(cellnrmax+1, (Integer)end);

       for (Index index = begin; index < end; ++index) {
           auto verts = StructuredGridBase::cellVertices(index, dims);
           const unsigned char type = UnstructuredGrid::HEXAHEDRON;
           const auto numFaces = UnstructuredGrid::NumFaces[type];
           for (int f=0; f<numFaces; ++f) {
               const int nCorners = UnstructuredGrid::FaceSizes[type][f];
               for (int i=0; i<=nCorners; ++i) {
                   const Index v = verts[UnstructuredGrid::FaceVertices[type][f][i%nCorners]];
                   ocl.push_back(v);
               }
               oel.push_back(ocl.size());
           }
       }

       if (auto s = StructuredGrid::as(grid)) {
           out->d()->x[0] = s->d()->x[0];
           out->d()->x[1] = s->d()->x[1];
           out->d()->x[2] = s->d()->x[2];
       } else {
           const Index numVert = str->getNumVertices();
           out->setSize(numVert);

           for (Index i=0; i<numVert; ++i) {

           }
       }
   }

   out->copyAttributes(grid);
   addObject("grid_out",out);

   return true;
}
