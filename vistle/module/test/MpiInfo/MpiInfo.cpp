#include <sstream>
#include <limits>
#include <algorithm>

#include <boost/mpl/for_each.hpp>
#include <boost/mpi/collectives/all_reduce.hpp>

#include <module/module.h>
#include <core/vec.h>
#include <core/scalars.h>
#include <core/message.h>
#include <core/coords.h>
#include <core/lines.h>
#include <core/triangles.h>
#include <core/indexed.h>
#include <util/hostname.h>

using namespace vistle;

class MpiInfo: public vistle::Module {

 public:
   MpiInfo(const std::string &shmname, const std::string &name, int moduleID);
   ~MpiInfo();

 private:

   virtual bool compute();
   virtual bool reduce(int timestep);

   bool prepare();
};

using namespace vistle;

MpiInfo::MpiInfo(const std::string &shmname, const std::string &name, int moduleID)
   : Module("MPI info", shmname, name, moduleID)
{
   std::stringstream str;
   str << "ctor: rank " << rank() << "/" << size() << " on host " << hostname() << std::endl;
   sendInfo(str.str());
}

MpiInfo::~MpiInfo() {

}

bool MpiInfo::compute() {

   std::stringstream str;
   str << "compute(): rank " << rank() << "/" << size() << " on host " << hostname() << std::endl;
   sendInfo(str.str());
   if (rank() == 0) {
       int len = 0;
       char version[MPI_MAX_LIBRARY_VERSION_STRING];
       MPI_Get_library_version(version, &len);
       std::stringstream str;
       str << "MPI version: " << std::string(version, len) << std::endl;
       sendInfo(str.str());
   }

   return true;
}

bool MpiInfo::prepare() {

   return true;
}

bool MpiInfo::reduce(int timestep) {

   return Module::reduce(timestep);
}

MODULE_MAIN(MpiInfo)

