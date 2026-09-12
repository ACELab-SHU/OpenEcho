#include "mem/venus_vrf_mem.hh"
#include "venus/venus_extension_pkg.hh"
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
namespace gem5
{
    namespace memory
    {
        namespace
        {
            constexpr Addr VenusVrfDataBase = 0x80100000;

            Addr
            dataBankBase(const std::vector<BackingStoreEntry> &storeEntries)
            {
                Addr base = std::numeric_limits<Addr>::max();
                for (const auto &entry : storeEntries) {
                    if (entry.range.start() >= VenusVrfDataBase)
                        base = std::min(base, entry.range.start());
                }
                if (base != std::numeric_limits<Addr>::max())
                    return base;

                for (const auto &entry : storeEntries)
                    base = std::min(base, entry.range.start());
                return base;
            }

            uint8_t
            readByteAt(const std::vector<BackingStoreEntry> &storeEntries,
                       Addr addr)
            {
                for (const auto &entry : storeEntries) {
                    if (entry.range.contains(addr)) {
                        const uint8_t value =
                            entry.pmem[addr - entry.range.start()];
                        const char *watch =
                            std::getenv("VENUS_GEM5_WATCH_PHYS_ADDR");
                        if (watch != nullptr &&
                            addr == std::strtoull(watch, nullptr, 0)) {
                            std::cerr << "[VRF watch] tick=" << curTick()
                                      << " read physical=0x" << std::hex
                                      << addr << " value=0x"
                                      << unsigned(value) << std::dec << '\n';
                        }
                        return value;
                    }
                }
                std::cerr << "Error: Backdoor read cannot find address 0x"
                          << std::hex << addr << std::dec << std::endl;
                return 0;
            }

            void
            writeByteAt(const std::vector<BackingStoreEntry> &storeEntries,
                        Addr addr, uint8_t value)
            {
                for (const auto &entry : storeEntries) {
                    if (entry.range.contains(addr)) {
                        const char *watch =
                            std::getenv("VENUS_GEM5_WATCH_PHYS_ADDR");
                        if (watch != nullptr &&
                            addr == std::strtoull(watch, nullptr, 0)) {
                            std::cerr << "[VRF watch] tick=" << curTick()
                                      << " write physical=0x" << std::hex
                                      << addr << " old=0x"
                                      << unsigned(entry.pmem[
                                             addr - entry.range.start()])
                                      << " new=0x" << unsigned(value)
                                      << std::dec << '\n';
                        }
                        entry.pmem[addr - entry.range.start()] = value;
                        return;
                    }
                }
                std::cerr << "Error: Backdoor write cannot find address 0x"
                          << std::hex << addr << std::dec << std::endl;
            }

            Addr
            dataElementAddr(const std::vector<BackingStoreEntry> &storeEntries,
                            int headline, int elem_idx, int vew,
                            int lane_num, int bank_num, int line_num)
            {
                const int data_per_bank = (vew == EW16) ? 1 : 2;
                const int elems_per_row = data_per_bank * bank_num * lane_num;
                const int index_within_row = elem_idx % elems_per_row;
                const int row_num = elem_idx / elems_per_row;
                const int lane_id =
                    (index_within_row / (data_per_bank * bank_num)) %
                    lane_num;
                const int bank_index_inlane =
                    (index_within_row / data_per_bank) % bank_num;
                const int bank_index =
                    (bank_index_inlane + row_num + headline) % bank_num;
                const Addr base = dataBankBase(storeEntries);
                const Addr lane_span = bank_num * line_num * 2;
                const Addr bank_span = line_num * 2;
                const Addr byte_offset =
                    (vew == EW8) ? (index_within_row % 2) : 0;

                const int effective_row =
                    (headline + row_num) & (line_num - 1);
                return base + lane_id * lane_span + bank_index * bank_span +
                    effective_row * 2 + byte_offset;
            }

            Addr
            lduElementAddr(const std::vector<BackingStoreEntry> &storeEntries,
                           int headline, int elem_idx, int vew,
                           int lane_num, int bank_num, int line_num)
            {
                // vldu.sv forms one VRF word as lane-major bytes: every lane
                // owns DataWidthB (NrBankPerLane * ELEN/8) consecutive bytes.
                // seq_word_wr_offset then increments the encoded VRF address
                // once per complete word.  venus_operand_requester.sv rotates
                // each logical bank by the low bank bits of that address.
                const int bytes_per_element = (vew == EW8) ? 1 : 2;
                const int bytes_per_lane = bank_num * 2;
                const int bytes_per_word = lane_num * bytes_per_lane;
                const int byte_index = elem_idx * bytes_per_element;
                const int word_offset = byte_index / bytes_per_word;
                const int byte_in_word = byte_index % bytes_per_word;
                const int lane_id = byte_in_word / bytes_per_lane;
                const int byte_in_lane = byte_in_word % bytes_per_lane;
                const int logical_bank = byte_in_lane / 2;
                const int encoded_addr = headline + word_offset;
                const int bank_index =
                    (logical_bank + (encoded_addr & (bank_num - 1))) % bank_num;
                const Addr base = dataBankBase(storeEntries);
                const Addr lane_span = bank_num * line_num * 2;
                const Addr bank_span = line_num * 2;
                const Addr byte_offset = byte_in_lane & 0x1;

                const int effective_row = encoded_addr & (line_num - 1);
                return base + lane_id * lane_span + bank_index * bank_span +
                    effective_row * 2 + byte_offset;
            }

            Addr
            vspmLogicalByteAddr(
                const std::vector<BackingStoreEntry> &storeEntries,
                Addr logicalAddr, int laneNum, int bankNum, int lineNum)
            {
                // venus_mem2lanes.sv consumes only MEM_WIDTH low address
                // bits. From LSB upward those fields are byte, logical bank,
                // lane, and row. The physical bank is barber-poled by the
                // low bank-width bits of the row.
                const unsigned byteBits = 1;
                unsigned bankBits = 0;
                unsigned laneBits = 0;
                unsigned rowBits = 0;
                for (unsigned value = bankNum; value > 1; value >>= 1)
                    ++bankBits;
                for (unsigned value = laneNum; value > 1; value >>= 1)
                    ++laneBits;
                for (unsigned value = lineNum; value > 1; value >>= 1)
                    ++rowBits;
                const unsigned memWidth =
                    byteBits + bankBits + laneBits + rowBits;
                const Addr offset = logicalAddr & ((Addr(1) << memWidth) - 1);
                const unsigned byte = offset & 1;
                const unsigned logicalBank =
                    (offset >> byteBits) & (bankNum - 1);
                const unsigned lane =
                    (offset >> (byteBits + bankBits)) & (laneNum - 1);
                const unsigned row =
                    (offset >> (byteBits + bankBits + laneBits)) &
                    (lineNum - 1);
                const unsigned physicalBank =
                    (logicalBank + (row & (bankNum - 1))) & (bankNum - 1);
                const Addr base = dataBankBase(storeEntries);
                const Addr laneSpan = bankNum * lineNum * 2;
                const Addr bankSpan = lineNum * 2;
                return base + lane * laneSpan + physicalBank * bankSpan +
                    row * 2 + byte;
            }

            Addr
            maskElementAddr(const std::vector<BackingStoreEntry> &storeEntries,
                            int elem_idx, int vew,
                            int lane_num, int bank_num, int line_num)
            {
                const int data_per_bank = (vew == EW16) ? 1 : 2;
                const int lane_id =
                    (elem_idx / (data_per_bank * bank_num)) % lane_num;
                const int elems_per_row =
                    data_per_bank * bank_num * lane_num;
                const int row_num = elem_idx / elems_per_row;
                const int elem_in_lane =
                    elem_idx % (data_per_bank * bank_num);
                const int byte_within_row = elem_in_lane * (vew + 1);
                const Addr base = dataBankBase(storeEntries);
                const Addr data_span =
                    lane_num * bank_num * line_num * 2;
                const Addr lane_span = bank_num * line_num * 2;

                return base + data_span + lane_id * lane_span +
                    row_num * bank_num * 2 + byte_within_row;
            }
        }

        // std::vector<AbstractMemory *> bank_mem; // 多个bank
        // PhysicalMemory *physMem;                // 一个 PhysicalMemory 实例，包含所有 SimpleMemory

        // 构造函数，传入简单内存的个数
        venus_vrf_mem::LogicalVspmPort::LogicalVspmPort(
            const std::string &name, venus_vrf_mem &owner)
            : SimpleTimingPort(name, &owner), owner(owner)
        {}

        Tick
        venus_vrf_mem::LogicalVspmPort::recvAtomic(PacketPtr pkt)
        {
            return owner.accessLogicalPort(pkt);
        }

        AddrRangeList
        venus_vrf_mem::LogicalVspmPort::getAddrRanges() const
        {
            return owner.logicalAddrRanges();
        }

        venus_vrf_mem::venus_vrf_mem(const venus_vrf_memParams &p) : SimObject(p),
                                                                     physMem(name() + ".physmem", p.memories, p.mmap_using_noreserve,
                                                                             p.shared_backstore, p.auto_unlink_shared_backstore),
                                                                     lane_num(p.lane_num),
                                                                     bank_num(p.bank_num),
                                                                     line_num(p.line_num),
                                                                     logicalBase(p.logical_base),
                                                                     logicalPort(name() + ".logical_port", *this),
                                                                     logicalLatency(p.logical_latency)
        {
            // uint8_t data1[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
            // backdoor_Write(0x100, sizeof(data1), data1);
            backdoor_Read();
            backdoor_Read();
        }

        Port &
        venus_vrf_mem::getPort(const std::string &if_name, PortID idx)
        {
            if (if_name == "logical_port")
                return logicalPort;
            return SimObject::getPort(if_name, idx);
        }

        void
        venus_vrf_mem::init()
        {
            SimObject::init();
            fatal_if(!logicalPort.isConnected(),
                     "Venus logical VSPM port is not connected");
            logicalPort.sendRangeChange();
        }

        AddrRangeList
        venus_vrf_mem::logicalAddrRanges() const
        {
            // Every hardware tile owns a distinct 2 MiB block window.  The
            // scalar core is RV32, so syscall-emulation may present either
            // zero-extended or sign-extended physical addresses.
            constexpr Addr vspmSize = 0x000ff000ULL;
            const Addr vspmEnd = logicalBase + vspmSize;
            const Addr signExtendedBase =
                0xffffffff00000000ULL | (logicalBase & 0xffffffffULL);
            const Addr signExtendedVspmEnd = signExtendedBase + vspmSize;
            return {
                AddrRange(logicalBase, vspmEnd),
                AddrRange(signExtendedBase, signExtendedVspmEnd),
            };
        }

        Tick
        venus_vrf_mem::accessLogicalPort(PacketPtr pkt)
        {
            const Addr addr = pkt->getAddr();
            const Addr addr32 = addr & 0xffffffffULL;

            // venus_block_wrapper routes [BLOCK_VSPM_OFFSET,
            // BLOCK_CTRLREGS_OFFSET) through venus_mem2lanes.  That module
            // converts the scalar/DMA byte stream into lane/bank/row SRAM
            // accesses and applies the row-dependent barber-pole rotation.
            const Addr logicalBase32 = logicalBase & 0xffffffffULL;
            const bool isVspm = addr32 >= logicalBase32 &&
                                addr32 < logicalBase32 + 0x000ff000ULL;
            if (pkt->isRead()) {
                uint8_t *data = pkt->getPtr<uint8_t>();
                if (isVspm)
                    backdoor_ReadVspm(addr32, pkt->getSize(), data);
                else
                    backdoor_Read(addr, pkt->getSize(), data);
            } else if (pkt->isWrite()) {
                const uint8_t *data = pkt->getConstPtr<uint8_t>();
                if (pkt->isMaskedWrite()) {
                    // A scheduler narrow DMA writes one aligned 64-byte AXI
                    // beat and uses WSTRB for its logical payload.  VSPM is
                    // non-linear in physical backing storage, so merge the
                    // mask in logical address order before committing it.
                    std::vector<uint8_t> merged(pkt->getSize());
                    if (isVspm)
                        backdoor_ReadVspm(addr32, merged.size(), merged.data());
                    else
                        backdoor_Read(addr, merged.size(), merged.data());
                    const auto &byteEnable = pkt->req->getByteEnable();
                    fatal_if(byteEnable.size() != merged.size(),
                             "Venus VSPM masked write byte-enable size mismatch");
                    for (size_t i = 0; i < merged.size(); ++i) {
                        if (byteEnable[i])
                            merged[i] = data[i];
                    }
                    if (isVspm)
                        backdoor_WriteVspm(addr32, merged.size(), merged.data());
                    else
                        backdoor_Write(addr, merged.size(), merged.data());
                } else if (isVspm) {
                    backdoor_WriteVspm(addr32, pkt->getSize(), data);
                } else {
                    backdoor_Write(addr, pkt->getSize(), data);
                }
            } else {
                panic("unsupported command %s on Venus logical VSPM port",
                      pkt->cmdString());
            }

            if (std::getenv("VENUS_GEM5_VSPM_TRACE") != nullptr && isVspm &&
                (std::getenv("VENUS_GEM5_VSPM_SCALAR_TRACE_ALL") != nullptr ||
                 (addr32 & 0x3fff) < 0x40)) {
                std::cerr << "[VSPM scalar] tick=" << curTick()
                          << " cmd=" << (pkt->isRead() ? "read" : "write")
                          << " logical=0x" << std::hex << addr32
                          << " size=" << std::dec << pkt->getSize()
                          << " data=";
                const uint8_t *data = pkt->getConstPtr<uint8_t>();
                for (unsigned i = 0; i < pkt->getSize(); ++i)
                    std::cerr << std::hex << std::setw(2)
                              << std::setfill('0') << unsigned(data[i]);
                std::cerr << std::dec << '\n';
            }

            if (pkt->needsResponse())
                pkt->makeResponse();
            return logicalLatency;
        }

        std::string venus_vrf_mem::backdoor_Read_Line(int headline, int vl, int vew)
        {
            std::stringstream ss;
            //input headline, vl, vew
            //output if vew==EW16 "DATA1[15:0]\nDATA2[15:0]\nDATA3[15:0]\nDATA4[15:0]\n..."
            //output if vew==EW8  "DATA1[7:0]\nDATA1[15:8]\nDATA2[7:0]\nDATA2[15:8]\n..."
            for (int i = 0; i < vl; i++) {
                // 根据 vew 决定读多少 byte
                if (vew == EW8) {
                    uint8_t v = backdoor_Read_Element(headline, i, vew);
                    ss << static_cast<uint32_t>(v) << "\n";  // cast to int 避免当 char 打印 :contentReference[oaicite:1]{index=1}
                } else { // EW16
                    ss << static_cast<uint32_t>(
                        backdoor_Read_Element(headline, i, vew)) << "\n";
                }
            }
            return ss.str();
        }

        std::string venus_vrf_mem::backdoor_Read_Mask(int vl, int vew)
        {
            std::stringstream ss;
            std::vector<BackingStoreEntry> storeEntries = physMem.getBackingStore();
            //input headline, vl, vew
            //output if vew==EW16 "DATA1[15:0]\nDATA2[15:0]\nDATA3[15:0]\nDATA4[15:0]\n..."
            //output if vew==EW8  "DATA1[7:0]\nDATA1[15:8]\nDATA2[7:0]\nDATA2[15:8]\n..."
            for (int i = 0; i < vl; i++) {
                Addr addr = maskElementAddr(storeEntries, i, vew, lane_num,
                                            bank_num, line_num);
                DPRINTF(MEMVRF,"i=%d: mask_addr=0x%x\n", i, addr);

                // 根据 vew 决定读多少 byte
                if (vew == EW8) {
                    uint8_t v = readByteAt(storeEntries, addr);
                    ss << static_cast<uint32_t>(v) << "\n";  // cast to int 避免当 char 打印 :contentReference[oaicite:1]{index=1}
                } else { // EW16
                    uint8_t lo = readByteAt(storeEntries, addr);
                    uint8_t hi = readByteAt(storeEntries, addr + 1);
                    uint16_t v16 = static_cast<uint16_t>( lo & hi );
                    ss << static_cast<uint32_t>(v16) << "\n";
                }
            }
            return ss.str();
        }

        uint16_t
        venus_vrf_mem::backdoor_Read_Element(int headline, int elem_idx, int vew)
        {
            std::vector<BackingStoreEntry> storeEntries = physMem.getBackingStore();
            const Addr addr = dataElementAddr(storeEntries, headline, elem_idx,
                                              vew, lane_num, bank_num,
                                              line_num);
            if (vew == EW8)
                return readByteAt(storeEntries, addr);
            uint8_t lo = readByteAt(storeEntries, addr);
            uint8_t hi = readByteAt(storeEntries, addr + 1);
            return static_cast<uint16_t>(lo | (hi << 8));
        }

        uint16_t
        venus_vrf_mem::backdoor_Read_Ldu_Element(int headline, int elem_idx, int vew)
        {
            std::vector<BackingStoreEntry> storeEntries = physMem.getBackingStore();
            const Addr addr = lduElementAddr(storeEntries, headline, elem_idx,
                                             vew, lane_num, bank_num,
                                             line_num);
            if (vew == EW8)
                return readByteAt(storeEntries, addr);
            uint8_t lo = readByteAt(storeEntries, addr);
            uint8_t hi = readByteAt(storeEntries, addr + 1);
            return static_cast<uint16_t>(lo | (hi << 8));
        }

        void
        venus_vrf_mem::backdoor_Write_Element(int headline, int elem_idx, int vew, uint16_t value)
        {
            std::vector<BackingStoreEntry> storeEntries = physMem.getBackingStore();
            const Addr addr = dataElementAddr(storeEntries, headline, elem_idx,
                                              vew, lane_num, bank_num,
                                              line_num);
            if (vew == EW8) {
                writeByteAt(storeEntries, addr, value & 0xff);
            } else {
                writeByteAt(storeEntries, addr, value & 0xff);
                writeByteAt(storeEntries, addr + 1, (value >> 8) & 0xff);
            }
        }

        void
        venus_vrf_mem::backdoor_Write_Ldu_Element(int headline, int elem_idx, int vew, uint16_t value)
        {
            std::vector<BackingStoreEntry> storeEntries = physMem.getBackingStore();
            const Addr addr = lduElementAddr(storeEntries, headline, elem_idx,
                                             vew, lane_num, bank_num,
                                             line_num);
            if (std::getenv("VENUS_GEM5_VSPM_TRACE") != nullptr &&
                headline == 0 && elem_idx < 64) {
                std::cerr << "[VSPM LDU] tick=" << curTick()
                          << " element=" << elem_idx
                          << " physical=0x" << std::hex << addr
                          << " value=0x" << value << std::dec << '\n';
            }
            if (vew == EW8) {
                writeByteAt(storeEntries, addr, value & 0xff);
            } else {
                writeByteAt(storeEntries, addr, value & 0xff);
                writeByteAt(storeEntries, addr + 1, (value >> 8) & 0xff);
            }
            if (std::getenv("VENUS_GEM5_VSPM_TRACE") != nullptr &&
                headline == 0 && elem_idx < 64) {
                std::cerr << "[VSPM LDU readback] tick=" << curTick()
                          << " element=" << elem_idx
                          << " physical=0x" << std::hex << addr
                          << " value=0x"
                          << unsigned(readByteAt(storeEntries, addr))
                          << std::dec << '\n';
            }
        }

        bool
        venus_vrf_mem::backdoor_Read_Mask_Element(int elem_idx, int vew)
        {
            std::vector<BackingStoreEntry> storeEntries = physMem.getBackingStore();
            Addr addr = maskElementAddr(storeEntries, elem_idx, vew,
                                        lane_num, bank_num, line_num);
            if (vew == EW8)
                return readByteAt(storeEntries, addr) != 0;
            uint8_t lo = readByteAt(storeEntries, addr);
            uint8_t hi = readByteAt(storeEntries, addr + 1);
            return (lo & hi) != 0;
        }

        // Function to print the contents of the first two memories
        void venus_vrf_mem::backdoor_Read()
        {
            // Retrieve the backing store entries
            std::vector<BackingStoreEntry> storeEntries = physMem.getBackingStore();
            DPRINTF(MEMVRF,"Backing store vector size: %d\n", storeEntries.size());
            for (size_t i = 0; i < storeEntries.size(); ++i)
            {
                // Print the address range for each memory
                const AddrRange &range = storeEntries[i].range;

                // Print the start and end addresses of the range
                DPRINTF(MEMVRF,"Memory %d (Address range: [%d - %d]):\n", i + 1, range.start(), range.end());
                if (::gem5::debug::MEMVRF) printMemoryContent(storeEntries[i].pmem, storeEntries[i].range.size());
            }
            // // Check if there are at least two memories
            // if (storeEntries.size() < 2)
            // {
            //     std::cerr << "Error: Less than two memories available in the backing store." << std::endl;
            //     return;
            // }

            // // Print the content of the first memory
            // std::cout << "Memory 1 (Address range: " << storeEntries[0].range << "):" << std::endl;
            // printMemoryContent(storeEntries[0].pmem, storeEntries[0].range.size());

            // // Print the content of the second memory
            // std::cout << "Memory 2 (Address range: " << storeEntries[1].range << "):" << std::endl;
            // printMemoryContent(storeEntries[1].pmem, storeEntries[1].range.size());
        }
        void
        venus_vrf_mem::backdoor_Read(Addr read_addr, size_t size, uint8_t *data)
        {
            std::vector<BackingStoreEntry> storeEntries = physMem.getBackingStore();
            for (size_t i = 0; i < size; ++i)
                data[i] = readByteAt(storeEntries, read_addr + i);
        }

        void
        venus_vrf_mem::backdoor_Write(Addr write_addr, size_t size, const uint8_t *data)
        {
            std::vector<BackingStoreEntry> storeEntries = physMem.getBackingStore();
            for (size_t i = 0; i < size; ++i)
                writeByteAt(storeEntries, write_addr + i, data[i]);
            DPRINTF(MEMVRF,"Backdoor Write successful to range: [0x%x - 0x%x], size: %d bytes.\n",write_addr, (write_addr + size - 1), size);
        }

        void
        venus_vrf_mem::backdoor_ReadVspm(Addr logical_addr, size_t size,
                                         uint8_t *data)
        {
            auto storeEntries = physMem.getBackingStore();
            for (size_t i = 0; i < size; ++i) {
                const Addr physical = vspmLogicalByteAddr(
                    storeEntries, logical_addr + i,
                    lane_num, bank_num, line_num);
                data[i] = readByteAt(storeEntries, physical);
                if (std::getenv("VENUS_GEM5_VSPM_TRACE") != nullptr &&
                    ((logical_addr + i) & 0x3fff) < 0x40) {
                    std::cerr << "[VSPM map] tick=" << curTick()
                              << " logical=0x" << std::hex
                              << (logical_addr + i)
                              << " physical=0x" << physical
                              << " value=0x" << unsigned(data[i])
                              << std::dec << '\n';
                }
            }
        }

        void
        venus_vrf_mem::backdoor_WriteVspm(Addr logical_addr, size_t size,
                                           const uint8_t *data)
        {
            auto storeEntries = physMem.getBackingStore();
            for (size_t i = 0; i < size; ++i) {
                const Addr physical = vspmLogicalByteAddr(
                    storeEntries, logical_addr + i,
                    lane_num, bank_num, line_num);
                writeByteAt(storeEntries, physical, data[i]);
            }
        }

        size_t
        venus_vrf_mem::persistentStateBytes() const
        {
            // One 16-bit entry per lane/bank/line exists in the data SRAM.
            // The mask SRAM has the same physical extent.  This evaluates to
            // 0x8000 for Venus2 (16x4x128) and 0x80000 for Venus1
            // (64x4x512), matching their RTL storage geometry.
            return 2ULL * static_cast<size_t>(lane_num) *
                   static_cast<size_t>(bank_num) *
                   static_cast<size_t>(line_num) * sizeof(uint16_t);
        }
        void venus_vrf_mem::backdoor_WriteRandom()
        {
            // 获取内存的 BackingStore
            std::vector<BackingStoreEntry> storeEntries = physMem.getBackingStore();

            // 随机数生成器初始化
            // std::srand(static_cast<unsigned>(std::time(0)));
            std::srand(1);

            DPRINTF(MEMVRF,"Starting random write operation...\n");

            for (auto &entry : storeEntries)
            {
                const AddrRange &range = entry.range;

                // 打印当前正在检查的内存范围
                DPRINTF(MEMVRF, "Checking range: [0x%x - 0x%x]\n", range.start(), (range.start() + range.size() - 1));

                // 仅在写入范围与 BackingStore 匹配时进行操作
                if (range.size() > 0)
                {
                    // 计算写入的大小（这里是整个内存范围的大小）
                    size_t write_size = range.size();

                    // 生成随机数据
                    std::vector<uint8_t> random_data(write_size);
                    for (size_t i = 0; i < write_size; ++i)
                    {
                        random_data[i] = static_cast<uint8_t>(std::rand() % 256); // 随机字节 [0, 255]
                    }

                    // 确保 entry.pmem 是有效的
                    if (entry.pmem == nullptr)
                    {
                        DPRINTF(MEMVRF, "Error: Invalid memory pointer for range: [0x%x - 0x%x]. Skipping this range.\n", range.start(), (range.start() + range.size() - 1));
                        continue; // 跳过无效的内存区域
                    }

                    // 确保写入数据不会超出范围
                    Addr offset = range.start();
                    if (offset + write_size > range.end())
                    {
                        DPRINTF(MEMVRF, "Error: Write operation exceeds memory range for [0x%x - 0x%x].\n", range.start(), range.end());
                        continue;
                    }

                    // 执行内存写入操作
                    std::memcpy(entry.pmem, random_data.data(), write_size);

                    DPRINTF(MEMVRF, "Backdoor Write successful to range: [0x%x - 0x%x], size: %d bytes.\n", range.start(), (range.start() + write_size - 1), write_size);
                }
            }

            DPRINTF(MEMVRF, "Random write operation completed.\n");
        }

        // Function to print the contents of memory
        void venus_vrf_mem::printMemoryContent(uint8_t *pmem, size_t size)
        {
            // Loop through the memory and print the content
            for (size_t i = 0; i < size; i++)
            {
                // Print data in hex format for clarity
                std::cout << std::setw(2) << std::setfill('0') << std::hex
                          << (int)pmem[i] << " ";

                // Print a newline every 16 bytes for readability
                if ((i + 1) % 16 == 0)
                {
                    std::cout << std::endl;
                }
            }

            // Print a final newline if the last line didn't end with one
            std::cout << std::endl;
        }

        // // 获取 PhysicalMemory 对象
        // PhysicalMemory venus_vrf_mem::getPhysicalMemory()
        // {
        //     return physMem;
        // }

        // 其他成员函数
    };
}
