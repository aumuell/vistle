#ifndef CELLTREE_H
#define CELLTREE_H

#include "export.h"
#include "scalar.h"
#include "index.h"
#include "shm.h"
#include "object.h"
#include "vector.h"
#include "geometry.h"
#include "shmvector.h"

namespace vistle {

// a bounding volume hierarchy, cf. C. Garth and K. I. Joy:
// “Fast, memory-efficient cell location in unstructured grids for visualization”,
// IEEE Transactions on Visualization and Computer Graphics, vol. 16, no. 6, pp. 1541–1550, 2010.

template<size_t IndexSize, int NumDimensions>
struct CelltreeNode;

template<typename Scalar, typename Index, int NumDimensions=3>
class V_COREEXPORT Celltree: public Object {
   V_OBJECT(Celltree);

 public:
   typedef Object Base;

   typedef typename VistleScalarVector<NumDimensions>::type Vector;
   typedef CelltreeNode<sizeof(Index), NumDimensions> Node;

   class VisitFunctor {

    public:
      enum Order {
         None = 0, //< none of the subnodes have to be visited
         RightFirst = 1, //< right subnode has to be visited, should be visited first
         RightSecond = 2, //< right subnode has to be visited, should be visited last
         Left = 4, //< left subnode has to be visited
         Right = RightFirst, //< right subnode has to be visited
         LeftRight = Left|RightSecond, //< both subnodes have to be visited, preferably left first
         RightLeft = Left|RightFirst, //< both subnodes have to be visited, preferably right first
      };

      //! check whether the celltree is within bounds min and max, otherwise no traversal
      bool checkBounds(const Scalar *min, const Scalar *max) {

         (void)min;
         (void)max;
         return true; // continue traversal
      }

      //! return whether and in which order to visit children of a node
      Order operator()(const Node &node) {

         (void)node;
         return LeftRight;
      }
   };

   //! return whether further cells have to be visited
   class LeafFunctor {
    public:
      bool operator()(Index elem) {
         return true; // continue traversal
      }
   };

   //! compute bounding box of a cell
   class CellBoundsFunctor {
    public:
      bool operator()(Index elem, Vector *min, Vector *max) {
         return false; // couldn't compute bounds
      }
   };

   Celltree(const Index numCells,
         const Meta &meta=Meta());

   void init(const Vector *min, const Vector *max, const Vector &gmin, const Vector &gmax);
   void refine(const Vector *min, const Vector *max, Index nodeIdx, const Vector &gmin, const Vector &gmax);
   template<class BoundsFunctor>
   bool validateTree(BoundsFunctor &func) const;

   Scalar *min() const { return &(*d()->m_bounds)[0]; }
   Scalar *max() const { return &(*d()->m_bounds)[NumDimensions]; }
   typename shm<Node>::array &nodes() { return *d()->m_nodes; }
   const typename shm<Node>::array &nodes() const { return *d()->m_nodes; }
   typename shm<Index>::array &cells() { return *d()->m_cells; }
   const typename shm<Index>::array &cells() const { return *d()->m_cells; }

   template<class InnerNodeFunctor, class ElementFunctor>
   void traverse(InnerNodeFunctor &visitNode, ElementFunctor &visitElement) const {
      if (!visitNode.checkBounds(min(), max()))
         return;
      traverseNode(0, nodes().data(), cells().data(), visitNode, visitElement);
   }

 private:
   template<class BoundsFunctor>
   bool validateNode(BoundsFunctor &func, Index nodenum, const Vector &min, const Vector &max) const;
   template<class InnerNodeFunctor, class ElementFunctor>
   bool traverseNode(Index curNode, const Node *nodes, const Index *cells, InnerNodeFunctor &visitNode, ElementFunctor &visitElement) const {

      const Node &node = nodes[curNode];
      if (node.isLeaf()) {
         for (Index i = node.start; i < node.start+node.size; ++i) {
            const Index cell = cells[i];
            if (!visitElement(cell))
               return false;
         }
         return true;
      }

      const typename VisitFunctor::Order order = visitNode(node);
      assert(!((order&VisitFunctor::RightFirst) && (order&VisitFunctor::RightSecond)));
      bool continueTraversal = true;
      if (continueTraversal && (order & VisitFunctor::RightFirst)) {
         continueTraversal = traverseNode(node.child+1, nodes, cells, visitNode, visitElement);
      }
      if (continueTraversal && (order & VisitFunctor::Left)) {
         continueTraversal = traverseNode(node.child, nodes, cells, visitNode, visitElement);
      }
      if (continueTraversal && (order & VisitFunctor::RightSecond)) {
         continueTraversal = traverseNode(node.child+1, nodes, cells, visitNode, visitElement);
      }

      return continueTraversal;
   }

   template<class Functor>
   bool traverseNode(Index curNode, Functor &func) const;

   V_DATA_BEGIN(Celltree);
      ShmVector<Scalar> m_bounds;
      ShmVector<Index> m_cells;
      ShmVector<Node> m_nodes;

      static Data *create(const std::string &name="", const Index numCells = 0,
            const Meta &m=Meta());
      Data(const std::string &name = "", const Index numCells = 0,
            const Meta &m=Meta());
   V_DATA_END(Celltree);
};

#include "celltreenode.h"

static_assert(sizeof(Celltree<Scalar, Index>::Node) % 8 == 0, "bad padding");

typedef Celltree<Scalar, Index, 1> Celltree1;
typedef Celltree<Scalar, Index, 2> Celltree2;
typedef Celltree<Scalar, Index, 3> Celltree3;

template<int Dim>
class V_COREEXPORT CelltreeInterface: virtual public ElementInterface {

 public:
   typedef vistle::Celltree<Scalar, Index, Dim> Celltree;
   virtual bool hasCelltree() const = 0;
   virtual typename Celltree::const_ptr getCelltree() const = 0;
   virtual bool validateCelltree() const = 0;
};

} // namespace vistle

namespace boost {
template<typename S, typename I, int d>
struct is_virtual_base_of<typename vistle::Celltree<S,I,d>::Base, vistle::Celltree<S,I,d>>: public mpl::true_ {};
}

#ifdef VISTLE_IMPL
// include only where actually required
//#include "celltree_impl.h"
#endif
#endif
