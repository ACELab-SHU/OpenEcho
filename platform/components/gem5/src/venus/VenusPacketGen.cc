#include "VenusPacketGen.hh"
#include "mem/packet.hh"

namespace gem5
{

// 与python间的链接函数
Port& VenusPacketGen::getPort(const std::string &if_name, PortID idx)
{
    // 检查请求的端口名称是否与 Python 文件中定义的 "port_venuspacketgen_sendto_venussequencer" 匹配
    if (if_name == "port_venuspacketgen_sendto_venussequencer") {
        return port_venuspacketgen_sendto_venussequencer; // 返回对应的端口对象
    }
    // 如果名称不匹配，则调用基类方法（可能会报错，但符合框架规范）
    return ClockedObject::getPort(if_name, idx);
}

// 端口发送
void VenusPacketGen::VenusPacketGenSequencerSidePort::sendPacket(PacketPtr pkt) {
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");
    if (!sendTimingReq(pkt)) {
        // std::cout << "at tick = " << curTick() << ", VenusPacketGenSequencerSidePort's sendTimingReq function get an nack." << std::endl;
        blockedPacket = pkt;
        isBlocked = true;
        return;
    }
    isBlocked = false;
}
// 端口重发
void VenusPacketGen::VenusPacketGenSequencerSidePort::recvReqRetry()
{
    assert(blockedPacket != nullptr);
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);
}

// 启动函数
void VenusPacketGen::startup() {
    schedule(nextTickEvent, afterCycles(Cycles(1))); // 首次调度
}

// 生成并操作端口
void VenusPacketGen::sendOneVenusPkt() {
    if(port_venuspacketgen_sendto_venussequencer.isBlocked == true) {
        // std::cout << "at tick = " << curTick() << ", VenusPacketGen initiate, resending last packet due to port_venuspacketgen_sendto_venussequencer.isBlocked is true. The content is:" << std::endl;
        // this->venus_instr_pkt->display();
        port_venuspacketgen_sendto_venussequencer.recvReqRetry();
    }
    else {
        // std::cout << "at tick = " << curTick() << ", VenusPacketGen initiate, generating new packet due to port_venuspacketgen_sendto_venussequencer.isBlocked is false." << std::endl;
        // this->venus_instr_pkt = new VenusInstrPkt();

        // //                                                                                         op      , vew, func3, vl, vs1_head, vs2_head, vd1_head, vd2_head, vm_r, scalar_op
        //      if(VenusInstrPkt::vns_instr_gencounter == 0) this->venus_instr_pkt = new VenusInstrPkt(VRANGE, EW16, OPMISC, 33, 0, 0, 64, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 1) this->venus_instr_pkt = new VenusInstrPkt(VRANGE, EW16, OPMISC, 33, 0, 0, 68, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 2) this->venus_instr_pkt = new VenusInstrPkt(VRANGE, EW16, OPMISC, 33, 0, 0, 164, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 3) this->venus_instr_pkt = new VenusInstrPkt(VRANGE, EW16, OPMISC, 33, 0, 0, 168, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 4) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 5) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW16, IVX, 33, 10, 64, 32, 0, false, 10);
        // else if(VenusInstrPkt::vns_instr_gencounter == 6) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW16, IVX, 33, 10, 68, 32, 0, false, 20);
        // else if(VenusInstrPkt::vns_instr_gencounter == 7) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW16, IVX, 33, 10, 168, 132, 0, false, 30);
        // else if(VenusInstrPkt::vns_instr_gencounter == 8) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 9) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW8, IVV, 33, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 10) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW8, IVV, 33, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 11) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW8, IVV, 33, 164, 168, 132, 0, false, 0);
        // else                                              this->venus_instr_pkt = new VenusInstrPkt();

        // //                                                                                          op    , vew , func3 , vl, vs1_head, vs2_head, vd1_head, vd2_head, vm_r, scalar_op
        //      if(VenusInstrPkt::vns_instr_gencounter ==  0) this->venus_instr_pkt = new VenusInstrPkt(VRANGE ,EW16, OPMISC, 1024, 0       , 0       , 0       , 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  1) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST,EW16,    IVX, 1024, 1       , 4       , 4       , 0, false, 4);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  2) this->venus_instr_pkt = new VenusInstrPkt(VREM   ,EW16,    IVV, 1024, 0       , 4       , 0       , 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  3) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST,EW16,    IVX, 1024, 1       , 10      , 10      , 0, false, 4);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  4) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter ==  5) this->venus_instr_pkt = new VenusInstrPkt(VSGT  , EW16,    MVX, 1024, 0       , 0       , 0       , 0, false, 3);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  6) this->venus_instr_pkt = new VenusInstrPkt(VADD  , EW16,    IVX, 1024, 0       , 0       , 100     , 0, true , 555);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  7) this->venus_instr_pkt = new VenusInstrPkt(VSLT  , EW16 ,   MVX, 1024, 0       , 0       , 0       , 0, true , 1);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  8) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter ==  9) this->venus_instr_pkt = new VenusInstrPkt(VADD  , EW8,    IVX, 1024, 0       , 0       , 100     , 0, true , 60);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  10) this->venus_instr_pkt = new VenusInstrPkt(VSGT  , EW8,    MVX, 1024, 0       , 0       , 0       , 0, false, 3);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  11) this->venus_instr_pkt = new VenusInstrPkt(VADD  , EW8,    IVX, 1024, 0       , 0       , 100     , 0, true , 50);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  12) this->venus_instr_pkt = new VenusInstrPkt(VSLT  , EW8 ,   MVX, 1024, 0       , 0       , 0       , 0, true , 1);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  13) this->venus_instr_pkt = new VenusInstrPkt(VADD  , EW16,    IVX, 1024, 0       , 0       , 100     , 0, true , 555);
        // else if(VenusInstrPkt::vns_instr_gencounter ==  14) this->venus_instr_pkt = nullptr;
        // else                                               this->venus_instr_pkt = new VenusInstrPkt();


        // //                                                                                       op     , vew  func3, vl, vs1_head, vs2_head, vd1_head, vd2_head, vm_r, scalar_op
        //      if(VenusInstrPkt::vns_instr_gencounter == 0) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 1024, 0      , 0       , 4      , 0       , false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 1) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 1024, 0      , 0       , 8      , 0       , false, 90);
        // else if(VenusInstrPkt::vns_instr_gencounter == 2) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 1024, 0      , 0       , 104    , 0       , false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 3) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 1024, 0      , 0       , 108    , 0       , false, 90);
        // else if(VenusInstrPkt::vns_instr_gencounter == 4) this->venus_instr_pkt = new VenusInstrPkt(VDIV   , EW8, IVX, 1024, 4      , 8       , 32     , 0       , false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 5) this->venus_instr_pkt = new VenusInstrPkt(VDIV   , EW8, IVX, 1024, 4      , 8       , 32     , 0       , false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 6) this->venus_instr_pkt = new VenusInstrPkt(VDIV   , EW8, IVX, 1024, 104    , 108     , 132    , 0       , false, 45);
        // else                                              this->venus_instr_pkt = new VenusInstrPkt();



        //                                                                                                op     , vew ,  func3,  vl, vs1_head, vs2_head, vd1_head, vd2_head, vm_r, scalar_op
                  if(VenusInstrPkt::vns_instr_gencounter == 0) this->venus_instr_pkt = new VenusInstrPkt(VRANGE  , EW16, OPMISC, 512, 0       , 0       , 0       , 0       , false, 0);
             else if(VenusInstrPkt::vns_instr_gencounter == 1) this->venus_instr_pkt = new VenusInstrPkt(VRSUB   , EW16, IVX   , 512, 0       , 0       , 256     , 0       , false, 256);
             else if(VenusInstrPkt::vns_instr_gencounter == 2) this->venus_instr_pkt = new VenusInstrPkt(VABS    , EW16, IVV   , 512, 0       , 256     , 384     , 0       , false, 0);
             else if(VenusInstrPkt::vns_instr_gencounter == 3) this->venus_instr_pkt = new VenusInstrPkt(VMIN    , EW16, IVX   , 512, 0       , 0       , 128     , 0       , false, 256);
             else if(VenusInstrPkt::vns_instr_gencounter == 4) this->venus_instr_pkt = new VenusInstrPkt(VMAX    , EW16, IVV   , 512, 128     , 256     , 0       , 0       , false, 0);
             else                                              this->venus_instr_pkt = new VenusInstrPkt();




        //      if(VenusInstrPkt::vns_instr_gencounter == 0) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 16, 16, 0, false, 16);
        // else if(VenusInstrPkt::vns_instr_gencounter == 1) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 17, 17, 0, false, 17);
        // else if(VenusInstrPkt::vns_instr_gencounter == 2) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 18, 18, 0, false, 18);
        // else if(VenusInstrPkt::vns_instr_gencounter == 3) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 19, 19, 0, false, 19);
        // else if(VenusInstrPkt::vns_instr_gencounter == 4) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 20, 20, 0, false, 20);
        // else if(VenusInstrPkt::vns_instr_gencounter == 5) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 21, 21, 0, false, 21);
        // else if(VenusInstrPkt::vns_instr_gencounter == 6) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 22, 22, 0, false, 22);
        // else if(VenusInstrPkt::vns_instr_gencounter == 7) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 23, 23, 0, false, 23);
        // else if(VenusInstrPkt::vns_instr_gencounter == 8) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 24, 24, 0, false, 24);
        // else if(VenusInstrPkt::vns_instr_gencounter == 9) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 25, 25, 0, false, 25);
        // else if(VenusInstrPkt::vns_instr_gencounter == 10) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 26, 26, 0, false, 26);
        // else if(VenusInstrPkt::vns_instr_gencounter == 11) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 27, 27, 0, false, 27);
        // else if(VenusInstrPkt::vns_instr_gencounter == 12) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 28, 28, 0, false, 28);
        // else if(VenusInstrPkt::vns_instr_gencounter == 13) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 29, 29, 0, false, 29);
        // else if(VenusInstrPkt::vns_instr_gencounter == 14) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 30, 30, 0, false, 30);
        // else if(VenusInstrPkt::vns_instr_gencounter == 15) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 31, 31, 0, false, 31);
        // else if(VenusInstrPkt::vns_instr_gencounter == 16) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 32, 32, 0, false, 32);
        // else if(VenusInstrPkt::vns_instr_gencounter == 17) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 33, 33, 0, false, 33);
        // else if(VenusInstrPkt::vns_instr_gencounter == 18) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 34, 34, 0, false, 34);
        // else if(VenusInstrPkt::vns_instr_gencounter == 19) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 35, 35, 0, false, 35);
        // else if(VenusInstrPkt::vns_instr_gencounter == 20) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 36, 36, 0, false, 36);
        // else if(VenusInstrPkt::vns_instr_gencounter == 21) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 37, 37, 0, false, 37);
        // else if(VenusInstrPkt::vns_instr_gencounter == 22) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 38, 38, 0, false, 38);
        // else if(VenusInstrPkt::vns_instr_gencounter == 23) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 39, 39, 0, false, 39);
        // else if(VenusInstrPkt::vns_instr_gencounter == 24) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 40, 40, 0, false, 40);
        // else if(VenusInstrPkt::vns_instr_gencounter == 25) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 41, 41, 0, false, 41);
        // else if(VenusInstrPkt::vns_instr_gencounter == 26) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 42, 42, 0, false, 42);
        // else if(VenusInstrPkt::vns_instr_gencounter == 27) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 43, 43, 0, false, 43);
        // else if(VenusInstrPkt::vns_instr_gencounter == 28) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 44, 44, 0, false, 44);
        // else if(VenusInstrPkt::vns_instr_gencounter == 29) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 45, 45, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 30) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 46, 46, 0, false, 46);
        // else if(VenusInstrPkt::vns_instr_gencounter == 31) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 47, 47, 0, false, 47);
        // else if(VenusInstrPkt::vns_instr_gencounter == 32) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 48, 48, 0, false, 48);
        // else if(VenusInstrPkt::vns_instr_gencounter == 33) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 49, 49, 0, false, 49);
        // else if(VenusInstrPkt::vns_instr_gencounter == 34) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 50, 50, 0, false, 50);
        // else if(VenusInstrPkt::vns_instr_gencounter == 35) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 51, 51, 0, false, 51);
        // else if(VenusInstrPkt::vns_instr_gencounter == 36) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 52, 52, 0, false, 52);
        // else if(VenusInstrPkt::vns_instr_gencounter == 37) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 53, 53, 0, false, 53);
        // else if(VenusInstrPkt::vns_instr_gencounter == 38) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 54, 54, 0, false, 54);
        // else if(VenusInstrPkt::vns_instr_gencounter == 39) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 55, 55, 0, false, 55);
        // else if(VenusInstrPkt::vns_instr_gencounter == 40) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 56, 56, 0, false, 56);
        // else if(VenusInstrPkt::vns_instr_gencounter == 41) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 57, 57, 0, false, 57);
        // else if(VenusInstrPkt::vns_instr_gencounter == 42) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 58, 58, 0, false, 58);
        // else if(VenusInstrPkt::vns_instr_gencounter == 43) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 59, 59, 0, false, 59);
        // else if(VenusInstrPkt::vns_instr_gencounter == 44) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 60, 60, 0, false, 60);
        // else if(VenusInstrPkt::vns_instr_gencounter == 45) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 61, 61, 0, false, 61);
        // else if(VenusInstrPkt::vns_instr_gencounter == 46) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 62, 62, 0, false, 62);
        // else if(VenusInstrPkt::vns_instr_gencounter == 47) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 63, 63, 0, false, 63);
        // else if(VenusInstrPkt::vns_instr_gencounter == 48) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 64, 64, 0, false, 64);
        // else if(VenusInstrPkt::vns_instr_gencounter == 49) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 65, 65, 0, false, 65);
        // else if(VenusInstrPkt::vns_instr_gencounter == 50) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 66, 66, 0, false, 66);
        // else if(VenusInstrPkt::vns_instr_gencounter == 51) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 67, 67, 0, false, 67);
        // else if(VenusInstrPkt::vns_instr_gencounter == 52) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 68, 68, 0, false, 68);
        // else if(VenusInstrPkt::vns_instr_gencounter == 53) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 69, 69, 0, false, 69);
        // else if(VenusInstrPkt::vns_instr_gencounter == 54) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 70, 70, 0, false, 70);
        // else if(VenusInstrPkt::vns_instr_gencounter == 55) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 71, 71, 0, false, 71);
        // else if(VenusInstrPkt::vns_instr_gencounter == 56) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 116, 116, 0, false, 116);
        // else if(VenusInstrPkt::vns_instr_gencounter == 57) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 117, 117, 0, false, 117);
        // else if(VenusInstrPkt::vns_instr_gencounter == 58) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 118, 118, 0, false, 118);
        // else if(VenusInstrPkt::vns_instr_gencounter == 59) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 119, 119, 0, false, 119);
        // else if(VenusInstrPkt::vns_instr_gencounter == 60) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 120, 120, 0, false, 120);
        // else if(VenusInstrPkt::vns_instr_gencounter == 61) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 121, 121, 0, false, 121);
        // else if(VenusInstrPkt::vns_instr_gencounter == 62) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 122, 122, 0, false, 122);
        // else if(VenusInstrPkt::vns_instr_gencounter == 63) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 123, 123, 0, false, 123);
        // else if(VenusInstrPkt::vns_instr_gencounter == 64) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 124, 124, 0, false, 124);
        // else if(VenusInstrPkt::vns_instr_gencounter == 65) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 125, 125, 0, false, 125);
        // else if(VenusInstrPkt::vns_instr_gencounter == 66) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 126, 126, 0, false, 126);
        // else if(VenusInstrPkt::vns_instr_gencounter == 67) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 127, 127, 0, false, 127);
        // else if(VenusInstrPkt::vns_instr_gencounter == 68) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 128, 128, 0, false, 128);
        // else if(VenusInstrPkt::vns_instr_gencounter == 69) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 129, 129, 0, false, 129);
        // else if(VenusInstrPkt::vns_instr_gencounter == 70) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 130, 130, 0, false, 130);
        // else if(VenusInstrPkt::vns_instr_gencounter == 71) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 131, 131, 0, false, 131);
        // else if(VenusInstrPkt::vns_instr_gencounter == 72) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 132, 132, 0, false, 132);
        // else if(VenusInstrPkt::vns_instr_gencounter == 73) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 133, 133, 0, false, 133);
        // else if(VenusInstrPkt::vns_instr_gencounter == 74) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 134, 134, 0, false, 134);
        // else if(VenusInstrPkt::vns_instr_gencounter == 75) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 135, 135, 0, false, 135);
        // else if(VenusInstrPkt::vns_instr_gencounter == 76) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 136, 136, 0, false, 136);
        // else if(VenusInstrPkt::vns_instr_gencounter == 77) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 137, 137, 0, false, 137);
        // else if(VenusInstrPkt::vns_instr_gencounter == 78) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 138, 138, 0, false, 138);
        // else if(VenusInstrPkt::vns_instr_gencounter == 79) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 139, 139, 0, false, 139);
        // else if(VenusInstrPkt::vns_instr_gencounter == 80) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 140, 140, 0, false, 140);
        // else if(VenusInstrPkt::vns_instr_gencounter == 81) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 141, 141, 0, false, 141);
        // else if(VenusInstrPkt::vns_instr_gencounter == 82) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 142, 142, 0, false, 142);
        // else if(VenusInstrPkt::vns_instr_gencounter == 83) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 143, 143, 0, false, 143);
        // else if(VenusInstrPkt::vns_instr_gencounter == 84) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 144, 144, 0, false, 144);
        // else if(VenusInstrPkt::vns_instr_gencounter == 85) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 145, 145, 0, false, 145);
        // else if(VenusInstrPkt::vns_instr_gencounter == 86) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 146, 146, 0, false, 146);
        // else if(VenusInstrPkt::vns_instr_gencounter == 87) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 147, 147, 0, false, 147);
        // else if(VenusInstrPkt::vns_instr_gencounter == 88) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 148, 148, 0, false, 148);
        // else if(VenusInstrPkt::vns_instr_gencounter == 89) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 149, 149, 0, false, 149);
        // else if(VenusInstrPkt::vns_instr_gencounter == 90) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 150, 150, 0, false, 150);
        // else if(VenusInstrPkt::vns_instr_gencounter == 91) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 151, 151, 0, false, 151);
        // else if(VenusInstrPkt::vns_instr_gencounter == 92) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 152, 152, 0, false, 152);
        // else if(VenusInstrPkt::vns_instr_gencounter == 93) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 153, 153, 0, false, 153);
        // else if(VenusInstrPkt::vns_instr_gencounter == 94) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 154, 154, 0, false, 154);
        // else if(VenusInstrPkt::vns_instr_gencounter == 95) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 155, 155, 0, false, 155);
        // else if(VenusInstrPkt::vns_instr_gencounter == 96) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 156, 156, 0, false, 156);
        // else if(VenusInstrPkt::vns_instr_gencounter == 97) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 157, 157, 0, false, 157);
        // else if(VenusInstrPkt::vns_instr_gencounter == 98) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 158, 158, 0, false, 158);
        // else if(VenusInstrPkt::vns_instr_gencounter == 99) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 159, 159, 0, false, 159);
        // else if(VenusInstrPkt::vns_instr_gencounter == 100) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 160, 160, 0, false, 160);
        // else if(VenusInstrPkt::vns_instr_gencounter == 101) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 161, 161, 0, false, 161);
        // else if(VenusInstrPkt::vns_instr_gencounter == 102) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 162, 162, 0, false, 162);
        // else if(VenusInstrPkt::vns_instr_gencounter == 103) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 163, 163, 0, false, 163);
        // else if(VenusInstrPkt::vns_instr_gencounter == 104) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 164, 164, 0, false, 164);
        // else if(VenusInstrPkt::vns_instr_gencounter == 105) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 165, 165, 0, false, 165);
        // else if(VenusInstrPkt::vns_instr_gencounter == 106) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 166, 166, 0, false, 166);
        // else if(VenusInstrPkt::vns_instr_gencounter == 107) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 167, 167, 0, false, 167);
        // else if(VenusInstrPkt::vns_instr_gencounter == 108) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 168, 168, 0, false, 168);
        // else if(VenusInstrPkt::vns_instr_gencounter == 109) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 169, 169, 0, false, 169);
        // else if(VenusInstrPkt::vns_instr_gencounter == 110) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 170, 170, 0, false, 170);
        // else if(VenusInstrPkt::vns_instr_gencounter == 111) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 256, 10, 171, 171, 0, false, 171);
        // else if(VenusInstrPkt::vns_instr_gencounter == 112) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 113) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW16, IVX, 1024, 6, 68, 32, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 114) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW16, IVX, 1024, 6, 68, 32, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 115) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW16, IVX, 1024, 6, 168, 132, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 116) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 117) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW8, IVV, 1024, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 118) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW8, IVV, 1024, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 119) this->venus_instr_pkt = new VenusInstrPkt(VAND, EW8, IVV, 1024, 164, 168, 132, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 120) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 121) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 1024, 6, 64, 64, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 122) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 1024, 6, 64, 64, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 123) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 1024, 6, 164, 164, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 124) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 125) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 1024, 6, 64, 64, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 126) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 1024, 6, 64, 64, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 127) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW16, IVX, 1024, 6, 164, 164, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 128) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 129) this->venus_instr_pkt = new VenusInstrPkt(VSLL, EW16, IVX, 1024, 6, 64, 32, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 130) this->venus_instr_pkt = new VenusInstrPkt(VSLL, EW16, IVX, 1024, 6, 64, 32, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 131) this->venus_instr_pkt = new VenusInstrPkt(VSLL, EW16, IVX, 1024, 6, 164, 132, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 132) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 133) this->venus_instr_pkt = new VenusInstrPkt(VSLL, EW8, IVV, 1024, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 134) this->venus_instr_pkt = new VenusInstrPkt(VSLL, EW8, IVV, 1024, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 135) this->venus_instr_pkt = new VenusInstrPkt(VSLL, EW8, IVV, 1024, 164, 168, 132, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 136) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 137) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW16, IVX, 1024, 6, 64, 32, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 138) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW16, IVX, 1024, 6, 64, 32, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 139) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW16, IVX, 1024, 6, 164, 132, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 140) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 141) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW8, IVV, 1024, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 142) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW8, IVV, 1024, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 143) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW8, IVV, 1024, 164, 168, 132, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 144) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 145) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW16, MVX, 1024, 6, 64, 0, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 146) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW16, MVX, 1024, 6, 64, 0, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 147) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW16, MVX, 1024, 6, 164, 0, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 148) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 149) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW8, MVV, 1024, 64, 68, 0, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 150) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW8, MVV, 1024, 64, 68, 0, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 151) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW8, MVV, 1024, 164, 168, 0, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 152) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 153) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW16, MVX, 1024, 6, 64, 0, 0, true, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 154) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW16, MVX, 1024, 6, 64, 0, 0, true, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 155) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW16, MVX, 1024, 6, 164, 0, 0, true, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 156) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 157) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW8, MVV, 1024, 64, 68, 0, 0, true, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 158) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW8, MVV, 1024, 64, 68, 0, 0, true, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 159) this->venus_instr_pkt = new VenusInstrPkt(VSEQ, EW8, MVV, 1024, 164, 168, 0, 0, true, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 160) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 161) this->venus_instr_pkt = new VenusInstrPkt(VRANGE, EW16, OPMISC, 1024, 0, 0, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 162) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 163) this->venus_instr_pkt = new VenusInstrPkt(VSUB, EW16, IVX, 1024, 6, 68, 32, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 164) this->venus_instr_pkt = new VenusInstrPkt(VSUB, EW16, IVX, 1024, 6, 68, 32, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 165) this->venus_instr_pkt = new VenusInstrPkt(VSUB, EW16, IVX, 1024, 6, 168, 132, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 166) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 167) this->venus_instr_pkt = new VenusInstrPkt(VSUB, EW8, IVV, 1024, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 168) this->venus_instr_pkt = new VenusInstrPkt(VSUB, EW8, IVV, 1024, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 169) this->venus_instr_pkt = new VenusInstrPkt(VSUB, EW8, IVV, 1024, 164, 168, 132, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 170) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 171) this->venus_instr_pkt = new VenusInstrPkt(VMUL, EW16, IVX, 1024, 6, 68, 32, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 172) this->venus_instr_pkt = new VenusInstrPkt(VMUL, EW16, IVX, 1024, 6, 68, 32, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 173) this->venus_instr_pkt = new VenusInstrPkt(VMUL, EW16, IVX, 1024, 6, 168, 132, 0, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 174) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 175) this->venus_instr_pkt = new VenusInstrPkt(VMUL, EW8, IVV, 1024, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 176) this->venus_instr_pkt = new VenusInstrPkt(VMUL, EW8, IVV, 1024, 64, 68, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 177) this->venus_instr_pkt = new VenusInstrPkt(VMUL, EW8, IVV, 1024, 164, 168, 132, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 178) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 179) this->venus_instr_pkt = new VenusInstrPkt(VMULADD, EW16, IVX, 1024, 6, 32, 16, 64, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 180) this->venus_instr_pkt = new VenusInstrPkt(VMULADD, EW16, IVX, 1024, 6, 32, 16, 64, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 181) this->venus_instr_pkt = new VenusInstrPkt(VMULADD, EW16, IVX, 1024, 6, 132, 116, 164, false, 180);
        // else if(VenusInstrPkt::vns_instr_gencounter == 182) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 183) this->venus_instr_pkt = new VenusInstrPkt(VMULADD, EW8, IVV, 1024, 32, 64, 16, 68, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 184) this->venus_instr_pkt = new VenusInstrPkt(VMULADD, EW8, IVV, 1024, 32, 64, 16, 68, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 185) this->venus_instr_pkt = new VenusInstrPkt(VMULADD, EW8, IVV, 1024, 132, 164, 116, 168, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 186) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 187) this->venus_instr_pkt = new VenusInstrPkt(VCMXMUL, EW16, IVV, 1024, 68, 64, 32, 16, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 188) this->venus_instr_pkt = new VenusInstrPkt(VCMXMUL, EW16, IVV, 1024, 38, 64, 32, 16, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 189) this->venus_instr_pkt = new VenusInstrPkt(VCMXMUL, EW16, IVV, 1024, 138, 164, 132, 116, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 190) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 191) this->venus_instr_pkt = new VenusInstrPkt(VCMXMUL, EW8, IVV, 1024, 68, 64, 32, 16, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 192) this->venus_instr_pkt = new VenusInstrPkt(VCMXMUL, EW8, IVV, 1024, 38, 64, 32, 16, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 193) this->venus_instr_pkt = new VenusInstrPkt(VCMXMUL, EW8, IVV, 1024, 138, 164, 132, 116, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 194) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 195) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 2048, 6, 190, 190, 0, false, 30);
        // else if(VenusInstrPkt::vns_instr_gencounter == 196) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 2048, 6, 290, 290, 0, false, 30);
        // else if(VenusInstrPkt::vns_instr_gencounter == 197) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 2048, 6, 200, 200, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 198) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 199) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 2048, 6, 300, 300, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 200) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 2048, 6, 210, 210, 0, false, 90);
        // else if(VenusInstrPkt::vns_instr_gencounter == 201) this->venus_instr_pkt = new VenusInstrPkt(VBRDCST, EW8, IVX, 2048, 6, 310, 310, 0, false, 90);
        // else if(VenusInstrPkt::vns_instr_gencounter == 202) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 203) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW16, IVX, 1024, 6, 190, 32, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 204) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW16, IVX, 1024, 6, 190, 32, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 205) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW16, IVX, 1024, 6, 290, 132, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 206) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 207) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW16, IVX, 1024, 6, 200, 32, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 208) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW16, IVX, 1024, 6, 200, 32, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 209) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW16, IVX, 1024, 6, 300, 132, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 210) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 211) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW16, IVX, 1024, 6, 210, 32, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 212) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW16, IVX, 1024, 6, 210, 32, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 213) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW16, IVX, 1024, 6, 310, 132, 0, false, 45);
        // else if(VenusInstrPkt::vns_instr_gencounter == 214) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 215) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW8, IVV, 1024, 200, 190, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 216) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW8, IVV, 1024, 200, 190, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 217) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW8, IVV, 1024, 300, 290, 132, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 218) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 219) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW8, IVV, 1024, 200, 200, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 220) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW8, IVV, 1024, 200, 200, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 221) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW8, IVV, 1024, 300, 300, 132, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 222) this->venus_instr_pkt = nullptr;
        // else if(VenusInstrPkt::vns_instr_gencounter == 223) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW8, IVV, 1024, 200, 210, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 224) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW8, IVV, 1024, 200, 210, 32, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 225) this->venus_instr_pkt = new VenusInstrPkt(VDIV, EW8, IVV, 1024, 300, 310, 132, 0, false, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 226) this->venus_instr_pkt = nullptr;
        // else                                                this->venus_instr_pkt = new VenusInstrPkt();

        // STABLE CAU TEST
        // if(VenusInstrPkt::vns_instr_gencounter == 0)
        // //                                             op      vew  func3  vl vs1 vs2 vd1 vd2
        //     this->venus_instr_pkt = new VenusInstrPkt(VADD   , EW16, IVV, 515, 0, 5, 100, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 1)
        // //                                             op      vew  func3  vl vs1 vs2 vd1 vd2
        //     this->venus_instr_pkt = new VenusInstrPkt(VMUL   , EW8, IVV, 515, 10, 15, 110, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 2)
        //                                             // op      vew  func3  vl vs1 vs2 vd1 vd2
        //     this->venus_instr_pkt = new VenusInstrPkt(VCMXMUL, EW16, IVV, 515, 20, 25, 120, 200);
        // else if(VenusInstrPkt::vns_instr_gencounter == 3)
        // //                                             op      vew  func3  vl vs1 vs2 vd1 vd2
        //     this->venus_instr_pkt = new VenusInstrPkt(VMUL   , EW16, IVV, 515, 30, 35, 130, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 4)
        // //                                             op      vew  func3  vl vs1 vs2 vd1 vd2
        //     this->venus_instr_pkt = new VenusInstrPkt(VADD   , EW8, IVV, 515, 40, 45, 140, 0);
        // else if(VenusInstrPkt::vns_instr_gencounter == 5)
        // //                                             op      vew  func3  vl vs1 vs2 vd1 vd2
        //     this->venus_instr_pkt = new VenusInstrPkt(VCMXMUL, EW16, IVV, 515, 50, 55, 150, 210);
        // else if(VenusInstrPkt::vns_instr_gencounter == 6)
        // //                                             op      vew  func3  vl vs1 vs2 vd1 vd2
        //     this->venus_instr_pkt = new VenusInstrPkt(VADD   , EW16, IVV, 515, 60, 65, 160, 0);
        // else
        //     this->venus_instr_pkt = new VenusInstrPkt();

        // std::cout << "at tick = " << curTick() << ", venus packet generate complete. The content is:" << std::endl;
        // this->venus_instr_pkt->display();
        // std::cout << "at tick = " << curTick() << ", VenusPacketGen start sending." << std::endl;
        if(this->venus_instr_pkt != nullptr)
            port_venuspacketgen_sendto_venussequencer.sendPacket((PacketPtr)(this->venus_instr_pkt));
        else
        {
            if(::gem5::debug::VenusSequencer) std::cout<<"else if(VenusInstrPkt::vns_instr_gencounter == "<<VenusInstrPkt::vns_instr_gencounter<<") this->venus_instr_pkt = nullptr;"<<std::endl;
            VenusInstrPkt::vns_instr_gencounter++;
            schedule(nextTickEvent, afterCycles(Cycles(1000)));
            return;
        } // sleep 1000clk
    }

    if(port_venuspacketgen_sendto_venussequencer.isBlocked == true) {
        schedule(nextTickEvent, afterCycles(Cycles(reschedule_interval)));
    }
    else {
        if(this->venus_instr_pkt->vns_instr_id < generate_howmany_times - 1)
            schedule(nextTickEvent, afterCycles(Cycles(schedule_interval)));
        delete this->venus_instr_pkt; // 成功发出后必须释放
    }
}
// // 用于计算vlength
// int venuspacket::vlength_cal(int vew, int vl) {
//     int vlength = 0;
//     switch (vew) {
//         case 8:
//             vlength = vl / (NrLanes * NrBankPerLane * 2);
//             if ((vl % (NrLanes * NrBankPerLane * 2)) != 0)
//                 vlength += 1;
//             break;
//         case 16:
//             vlength = vl / (NrLanes * NrBankPerLane);
//             if ((vl % (NrLanes * NrBankPerLane)) != 0)
//                 vlength += 1;
//             break;
//         default:
//             break;
//     }
//     return vlength;
// }
}
