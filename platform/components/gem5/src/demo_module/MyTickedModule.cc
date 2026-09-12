#include "demo_module/MyTickedModule.hh"
#include "base/trace.hh"
#include "debug/MyTickedDebug.hh"
namespace gem5
{
MyTickedModule::MyTickedModule(const MyTickedModuleParams &params)
:   TickedObject(params),
    maxCount(params.max_count)

{
    DPRINTF(MyTickedDebug, "MyTickedModule created. Max count = %d\n", maxCount);
    start();  // 启动 ticking
}

void MyTickedModule::evaluate()
{
    DPRINTF(MyTickedDebug, "Tick at cycle %llu, counter = %d\n", curTick(), counter);
    counter++;

    if (counter >= maxCount) {
        DPRINTF(MyTickedDebug, "Reached max count. Descheduling.\n");
        // deschedule();
    }
}

void
MyTickedModule::regStats()
{
    TickedObject::regStats();  // 不要漏这一句！
    // Ticked::regStats();
}


// MyTickedModule *
// MyTickedModuleParams::create() const
// {
//     return new MyTickedModule(*this,static_cast<const ClockedObjectParams &>(*this));
// }
}


    // : Ticked(params), maxCount(params.max_count)