#ifndef THRESHOLD_H
#define THRESHOLD_H

#include <vistle/module/module.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/util/coRestraint.h>
#include <memory>

#ifdef CELLSELECT
#define Threshold CellSelect
#endif

class Threshold: public vistle::Module {
public:
    Threshold(const std::string &name, int moduleID, mpi::communicator comm);
    ~Threshold();

    typedef std::map<vistle::Index, vistle::Index> VerticesMapping;
    typedef std::vector<vistle::Index> ElementsMapping;

private:
    static const unsigned NUMPORTS = 3;
    bool compute(std::shared_ptr<vistle::BlockTask> task) const override;
    bool prepare() override;

    void renumberVertices(vistle::Coords::const_ptr coords, vistle::Indexed::ptr poly, VerticesMapping &vm) const;

    vistle::IntParameter *p_reuse = nullptr;
    bool m_invert = false;
    vistle::IntParameter *p_invert = nullptr;
#ifdef CELLSELECT
    vistle::coRestraint m_restraint;
    vistle::StringParameter *p_restraint = nullptr;
#else
    vistle::FloatParameter *p_threshold = nullptr;
    vistle::IntParameter *p_operation = nullptr;
#endif
    vistle::Port *p_in[NUMPORTS] = {};
    vistle::Port *p_out[NUMPORTS] = {};
};

#endif
