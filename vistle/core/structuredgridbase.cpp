//-------------------------------------------------------------------------
// STRUCTURED GRID OBJECT BASE CLASS CPP
// *
// * Base class for Structured Grid Objects
//-------------------------------------------------------------------------

#include "structuredgridbase.h"

namespace vistle {


// IS GHOST CELL CHECK
//-------------------------------------------------------------------------
bool StructuredGridBase::isGhostCell(Index elem) const {
      if (elem == InvalidIndex)
          return false;

      Index dims[3];
      std::array<Index,3> cellCoords;

      for (int c=0; c<3; ++c) {
          dims[c] = getNumDivisions(c);
      }

      cellCoords = cellCoordinates(elem, dims);

      for (int c=0; c<3; ++c) {
          if (cellCoords[c] < getNumGhostLayers(c, Bottom)
                  || cellCoords[c] >= (dims[c]-1)-getNumGhostLayers(c, Top)) {
              return true;
          }
      }

      return false;

}

Scalar StructuredGridBase::cellDiameter(Index elem) const {

    auto bounds = cellBounds(elem);
    return (bounds.second-bounds.first).norm();
}

std::vector<Index> StructuredGridBase::getNeighborElements(Index elem) const {

    std::vector<Index> elems;
    if (elem == InvalidIndex)
        return elems;

    const Index dims[3] = { getNumDivisions(0), getNumDivisions(1), getNumDivisions(2) };
    const auto coords = cellCoordinates(elem, dims);
    for (int d=0; d<3; ++d) {
        auto c = coords;
        if (coords[d] >= 1) {
            c[d] = coords[d]-1;
            elems.push_back(cellIndex(c[0], c[1], c[2], dims));
        }
        if (coords[d] < dims[d]-2) {
            c[d] = coords[d]+1;
            elems.push_back(cellIndex(c[0], c[1], c[2], dims));
        }
    }

    return elems;
}

} // namespace vistle
