#ifndef VISTLE_GHOSTCELLS_GHOSTCELLS_H
#define VISTLE_GHOSTCELLS_GHOSTCELLS_H


#include <vistle/module/module.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>

class GhostCells: public vistle::Module {
public:
    GhostCells(const std::string &name, int moduleID, mpi::communicator comm);
    ~GhostCells();

    typedef std::vector<vistle::Index> DataMapping;

private:
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
    bool reduce(int t) override;

    template<class Surface>
    struct Result {
        typename Surface::ptr surface;
        DataMapping surfaceElements;
        vistle::Lines::ptr lines;
        DataMapping lineElements;
    };

    Result<vistle::Polygons> createSurface(vistle::UnstructuredGrid::const_ptr m_grid_in, bool haveElementData,
                                           bool createSurface, bool createLines) const;
    Result<vistle::Quads> createSurface(vistle::StructuredGridBase::const_ptr m_grid_in, bool haveElementData,
                                        bool createSurface, bool createLines) const;
    void renumberVertices(vistle::Coords::const_ptr coords, vistle::Indexed::ptr poly, DataMapping &vm) const;
    void renumberVertices(vistle::Coords::const_ptr coords, vistle::Quads::ptr quad, DataMapping &vm) const;
    //bool checkNormal(vistle::Index v1, vistle::Index v2, vistle::Index v3, vistle::Scalar x_center, vistle::Scalar y_center, vistle::Scalar z_center);

    mutable vistle::ResultCache<vistle::Object::ptr> m_cache;

    struct BlockData {
        vistle::UnstructuredGrid::const_ptr grid;
        std::vector<vistle::Index> boundaryVertices;
        vistle::Vector3 min, max;
        std::vector<std::vector<vistle::Index>> candidateVertices; // outside index by foreign block number
        std::vector<vistle::Index> exchangeSize;
    };

    mutable std::mutex m_blockDataMutex;
    mutable std::map<int, BlockData> m_blockData;

    std::vector<vistle::Index> boundaryVertices(vistle::UnstructuredGrid::const_ptr m_grid_in) const;
};

#endif
