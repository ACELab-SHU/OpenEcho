  /*
   * Venus1.0 CCM write protocol, reconstructed instruction-for-instruction
   * from the repository-qualified simpledagfire firmware at a34a99a.
   * Venus2.0 uses different command heads and must keep the shared default.
   */
  volatile uint32_t r_rsp;
  r_rsp = READ_BURST_32(base_addr, offset_addr);
  WRITE_BURST_32(base_addr, offset_addr, RSP_CONCAT(0x02, r_rsp));
  WRITE_BURST_32(base_addr, offset_addr, RSP_CONCAT(0x03, r_rsp));
  WRITE_BURST_32(base_addr, offset_addr, RSP_CONCAT(0x03, target_settings));
  WRITE_BURST_32(base_addr, offset_addr, RSP_CONCAT(0x02, target_settings));

  for (volatile int wait = 0; wait < 1000; ++wait) {
  }

  volatile uint32_t response = READ_BURST_32(base_addr, offset_addr);
  while ((response & 0x80000000) != 0x80000000) {
    response = READ_BURST_32(base_addr, offset_addr);
  }
  WRITE_BURST_32(base_addr, offset_addr, target_settings & 0x00ffffff);
