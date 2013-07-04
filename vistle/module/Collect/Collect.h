#ifndef COLLECT_H
#define COLLECT_H

#include <module/module.h>

class Collect: public vistle::Module {

 public:
   Collect(const std::string &shmname, int rank, int size, int moduleID);
   ~Collect();

 private:

   virtual bool compute();
};

#endif
