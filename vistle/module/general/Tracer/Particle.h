#ifndef TRACER_PARTICLE_H
#define TRACER_PARTICLE_H

#include <vector>
#include <future>

#include <boost/mpi/communicator.hpp>

#include <util/enum.h>
#include <core/vector.h>
#include <core/scalars.h>
#include <core/index.h>
#include <core/lines.h>

#include "Integrator.h"

class BlockData;
class GlobalData;

class Particle {

    friend class Integrator;
    friend class boost::serialization::access;

public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(StopReason,
                                        (StillActive)
                                        (InitiallyOutOfDomain)
                                        (OutOfDomain)
                                        (NotMoving)
                                        (StepLimitReached)
                                        (DistanceLimitReached)
                                        (TimeLimitReached)
                                        (NumStopReasons)
    )
    Particle(vistle::Index id, int rank, vistle::Index startId, const vistle::Vector3 &pos, bool forward, GlobalData &global, vistle::Index timestep);
    ~Particle();
    vistle::Index id() const;
    int rank();
    bool isActive() const;
    bool inGrid() const;
    bool isMoving();
    bool isForward() const;
    void Deactivate(StopReason reason);
    void EmitData();
    bool Step();
    void broadcast(boost::mpi::communicator mpi_comm, int root);
    void sendData(boost::mpi::communicator mpi_comm);
    void receiveData(boost::mpi::communicator mpi_comm, int rank);
    void UpdateBlock(BlockData *block);
    StopReason stopReason() const;
    void enableCelltree(bool value);
    int startTracing(boost::mpi::communicator mpi_comm); //< returns MPI rank of node where tracing occurs
    bool isTracing(bool wait);
    bool madeProgress() const;
    bool trace();
    void finishSegment();
    void fetchSegments(Particle &other); //! move segments from other particle to this one
    void addToOutput();
    vistle::Scalar time() const;

private:
    bool findCell(double time);

    GlobalData &m_global;
    vistle::Index m_id; //!< particle id
    vistle::Index m_startId; //!< id of start point;
    int m_rank; //! MPI rank where resulting geometry is assembled
    vistle::Index m_timestep; //! timestep of particle for streamlines
    std::future<bool> m_progressFuture; //!< future on whether particle has made progress during trace()
    bool m_progress;
    bool m_tracing; //!< particle is currently tracing on this node
    bool m_forward; //!< trace direction
    vistle::Vector3 m_x; //!< current position
    vistle::Vector3 m_xold; //!< previous position
    vistle::Vector3 m_v; //!< current velocity
    vistle::Scalar m_p; //!< current pressure
    vistle::Index m_stp; //!< current integration step
    vistle::Scalar m_time; //! current time
    vistle::Scalar m_dist; //!< total distance travelled
    int m_segment; //!< number of segment being traced on this rank
    vistle::Index m_segmentStart; //! number of step where this segment started

    struct Segment {
        int m_rank;
        int m_num; // >= 0: forward, < 0: backward
        vistle::Index m_startStep;
        std::vector<vistle::Vector3> m_xhist; //!< trajectory
        std::vector<vistle::Vector3> m_vhist; //!< previous velocities
        std::vector<vistle::Scalar> m_pressures; //!< previous pressures
        std::vector<vistle::Index> m_steps; //!< previous steps
        std::vector<vistle::Scalar> m_times; //!< previous times
        std::vector<vistle::Scalar> m_dists; //!< previous times

        Segment(int num=0)
            : m_rank(-1)
            , m_num(num)
            , m_startStep(vistle::InvalidIndex)
        {}

        // just for Boost.MPI
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & m_rank;
            ar & m_num;
            ar & m_xhist;
            ar & m_vhist;
            ar & m_pressures;
            ar & m_steps;
            ar & m_times;
            ar & m_dists;
        }

        void clear() {
            m_rank = -1;
            m_num = 0;
            m_xhist.clear();
            m_vhist.clear();
            m_pressures.clear();
            m_steps.clear();
            m_times.clear();
            m_dists.clear();
        }
    };

    std::shared_ptr<Segment> m_currentSegment;
    std::map<int, std::shared_ptr<Segment>> m_segments;

    BlockData *m_block; //!< current block for current particle position
    vistle::Index m_el; //!< index of cell for current particle position
    bool m_ingrid; //!< particle still within domain on some rank
    Integrator m_integrator;
    StopReason m_stopReason; //! reason why particle was deactivated
    bool m_useCelltree; //! whether to use celltree for acceleration

    // just for Boost.MPI
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & m_x;
        ar & m_xold;
        ar & m_v;
        ar & m_stp;
        ar & m_time;
        ar & m_dist;
        ar & m_p;
        ar & m_ingrid;
        ar & m_integrator.m_h;
        ar & m_stopReason;
        ar & m_segment;
    }
};
#endif
