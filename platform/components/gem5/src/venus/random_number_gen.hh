// #ifndef __RANDOM_NUMBER_GEN_HH__
// #define __RANDOM_NUMBER_GEN_HH__

// #include "sim/sim_object.hh"
// #include "params/random_number_gen.hh"
// #include <queue>
// // #include "sim/clocked_object.hh"
// #include "venus/venuspacket.hh" // 包含 packet_gen 类头文件
// #include <random>

// namespace gem5 {
// class random_number_gen : public SimObject
// {
//   public:
//     random_number_gen(const random_number_genParams &params);

//     void init() override;
//     void generateNumber();
//   private:
//     // 用于调度的事件
//     EventFunctionWrapper nextTickEvent;
//     int cycle_count;

//     // FIFO 队列，用来存储 venuspacket 对象
//     std::queue<venuspacket> packetFifo;
//     // 随机生成数字的函数
//     int generateRandomNumber();
//     int randomCycleTime();
// };
// } // namespace gem5

// #endif // __RANDOM_NUMBER_GENERATOR_HH__
