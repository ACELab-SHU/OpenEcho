// #include "base/trace.hh"
// #include "venus/random_number_gen.hh"



// // #include "debug/random_number_gen.hh"
// #include <iostream>
// namespace gem5
// {

// const int FIFO_DEPTH = 5;  // 你可以根据需要调整队列深度
// random_number_gen::random_number_gen(const random_number_genParams &params) : SimObject(params), 
// nextTickEvent([this]{generateNumber();},name()),
// cycle_count(0),
// packetFifo() {
//     std::cout << "this is a test random number generate sequential function to simulate packet gen!\n" << std::endl;
//     // DPRINTF(RandomNumberGenerator,"overwrite generatenumber by packetgen in venus\n");
// }

//     // 注册函数
// void random_number_gen::init() {
//         schedule(nextTickEvent, curTick() + randomCycleTime()); // 首次调度
//     }

// void random_number_gen::generateNumber() {
//         int increase = randomCycleTime();
//         cycle_count = cycle_count + increase;
//         int randomNumber = generateRandomNumber();
//         std::cout << "Cycle: " << cycle_count << ", Random Number: " << randomNumber << std::endl;

//         if (packetFifo.size() < FIFO_DEPTH) {  // FIFO 未满
//             venuspacket m_venuspacket;  // 在此实例化一个新的 packet_gen
//             m_venuspacket.randomize();  // 生成并打印包的信息
//             m_venuspacket.display();
//             packetFifo.push(m_venuspacket);  // 将新的包推入 FIFO

//             std::cout << "Packet added to FIFO. FIFO size: " << packetFifo.size() << std::endl;
//         } else {
//             std::cout << "FIFO is full. No packet generated this cycle.\n";
//         }
//         // 重新调度下一个事件
//         schedule(nextTickEvent, curTick() + increase);
//     }

// // private:
// int random_number_gen::randomCycleTime() {
//         std::random_device rd;
//         std::mt19937 gen(rd());
//         std::uniform_int_distribution<> dist(1, 10);
//         return dist(gen);
//     }

// int random_number_gen::generateRandomNumber() {
//         std::random_device rd;
//         std::mt19937 gen(rd());
//         std::uniform_int_distribution<> dist(0, 100);
//         return dist(gen);
//     }
// }