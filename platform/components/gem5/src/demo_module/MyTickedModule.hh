#ifndef __MY_TICKED_MODULE_HH__
#define __MY_TICKED_MODULE_HH__

#include "sim/ticked_object.hh"
#include "params/MyTickedModule.hh"
#include "sim/clocked_object.hh"
namespace gem5{
class MyTickedModule : public TickedObject
{
  public:
    MyTickedModule(const MyTickedModuleParams &params);

  public:
    void evaluate() override;
    void regStats() override;
  private:
    int counter = 0;
    int maxCount;
};
}

#endif // __MY_TICKED_MODULE_HH__
