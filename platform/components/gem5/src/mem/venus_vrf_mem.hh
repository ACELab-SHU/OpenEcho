#ifndef VENUS_VRF_MEM_HH
#define VENUS_VRF_MEM_HH

#include <vector>
#include <string>
#include "debug/MEMVRF.hh"
#include "mem/abstract_mem.hh"
#include "mem/simple_mem.hh"
#include "mem/physical.hh"
#include "mem/tport.hh"
#include "params/SimpleMemory.hh" // 包含 SimpleMemoryParams
#include "params/venus_vrf_mem.hh"
#include <iomanip>
namespace gem5
{
    namespace memory
    {
        // venus_vrf_mem 类声明
        class venus_vrf_mem : public SimObject
        {
        private:
            class LogicalVspmPort : public SimpleTimingPort
            {
              private:
                venus_vrf_mem &owner;

              protected:
                Tick recvAtomic(PacketPtr pkt) override;
                AddrRangeList getAddrRanges() const override;

              public:
                LogicalVspmPort(const std::string &name,
                                venus_vrf_mem &owner);
            };

            // std::vector<AbstractMemory *> bank_mem; // 存储多个 SimpleMemory 对象的向量
            PhysicalMemory physMem; // 一个 PhysicalMemory 实例，包含所有 SimpleMemory
            int lane_num;
            int bank_num;
            int line_num;
            const Addr logicalBase;
            LogicalVspmPort logicalPort;
            const Tick logicalLatency;

            Tick accessLogicalPort(PacketPtr pkt);
            AddrRangeList logicalAddrRanges() const;
        public:
            // 构造函数，传入 venus_vrf_memParams 对象
            venus_vrf_mem(const venus_vrf_memParams &p);
            void init() override;
            Port &getPort(const std::string &if_name,
                          PortID idx=InvalidPortID) override;
            std::string backdoor_Read_Line(int headline, int vl, int vew);
            std::string backdoor_Read_Mask(int vl, int vew);
            uint16_t backdoor_Read_Element(int headline, int elem_idx, int vew);
            uint16_t backdoor_Read_Ldu_Element(int headline, int elem_idx, int vew);
            void backdoor_Write_Element(int headline, int elem_idx, int vew, uint16_t value);
            void backdoor_Write_Ldu_Element(int headline, int elem_idx, int vew, uint16_t value);
            bool backdoor_Read_Mask_Element(int elem_idx, int vew);
            void backdoor_Read();
            void backdoor_Read(Addr read_addr, size_t size, uint8_t *data);
            void backdoor_Write(Addr write_addr, size_t size, const uint8_t *data);
            void backdoor_ReadVspm(Addr logical_addr, size_t size,
                                   uint8_t *data);
            void backdoor_WriteVspm(Addr logical_addr, size_t size,
                                    const uint8_t *data);
            // Bytes of architectural VRF state that survive a task switch.
            // Both the data banks and the per-lane mask SRAM are persistent;
            // derive their extent from the configured Venus geometry instead
            // of assuming a Venus2-only fixed snapshot size.
            size_t persistentStateBytes() const;
            void printMemoryContent(uint8_t *pmem, size_t size);
            void backdoor_WriteRandom();
            // 获取 PhysicalMemory 对象
            // PhysicalMemory getPhysicalMemory();

            // 可以添加其他成员函数
        };
    }
}

#endif // GEM5_MEMORY_VENUS_VRF_MEM_HH
