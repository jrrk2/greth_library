-----------------------------------------------------------------------------
--! @file
--! @copyright  Copyright 2015 GNSS Sensor Ltd. All right reserved.
--! @author     Sergey Khabarov
--! @brief      Network on Chip design top level.
--! @details    RISC-V "Rocket Core" based system with the AMBA AXI4 (NASTI) 
--!             system bus and integrated peripheries.
------------------------------------------------------------------------------
--! Standard library
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

--! Data transformation and math functions library
library commonlib;
use commonlib.types_common.all;

--! Technology definition library.
library techmap;
--! Technology constants definition.
use techmap.gencomp.all;
--! "Virtual" PLL declaration.
use techmap.types_pll.all;
--! "Virtual" buffers declaration.
use techmap.types_buf.all;

--! AMBA system bus specific library
library ambalib;
--! AXI4 configuration constants.
use ambalib.types_amba4.all;

--! Rocket-chip specific library
library rocketlib;
--! SOC top-level component declaration.
use rocketlib.types_rocket.all;
--! Ethernet related declarations.
use rocketlib.grethpkg.all;

 --! Top-level implementaion library
library work;
--! Target dependable configuration: RTL, FPGA or ASIC.
use work.config_target.all;
--! Target independable configuration.
use work.config_common.all;

--! @brief   SOC Top-level entity declaration.
--! @details This module implements full SOC functionality and all IO signals
--!          are available on FPGA/ASIC IO pins.
entity rocket_soc is port 
( 
  --! Input reset. Active High. Usually assigned to button "Center".
  i_rst     : in std_logic;
  --! Differential clock (LVDS) positive signal.
  i_sclk_p  : in std_logic;
  --! Differential clock (LVDS) negative signal.
  i_sclk_n  : in std_logic;
  --! DIP switch.
  i_dip     : in std_logic_vector(3 downto 0);
  --! LEDs.
  o_led     : out std_logic_vector(7 downto 0);
  --! Ethernet MAC PHY interface signals
  o_erefclk   : out   std_ulogic; -- RMII clock out
  i_gmiiclk_p : in    std_ulogic; -- GMII clock in
  i_gmiiclk_n : in    std_ulogic;
  o_egtx_clk  : out   std_ulogic;
  i_etx_clk   : in    std_ulogic;
  i_erx_clk   : in    std_ulogic;
  i_erxd      : in    std_logic_vector(3 downto 0);
  i_erx_dv    : in    std_ulogic;
  i_erx_er    : in    std_ulogic;
  i_erx_col   : in    std_ulogic;
  i_erx_crs   : in    std_ulogic;
  i_emdint    : in    std_ulogic;
  o_etxd      : out   std_logic_vector(3 downto 0);
  o_etx_en    : out   std_ulogic;
  o_etx_er    : out   std_ulogic;
  o_emdc      : out   std_ulogic;
  io_emdio    : inout std_logic;
  o_erstn     : out   std_ulogic
);
  --! @}

end rocket_soc;

--! @brief SOC top-level  architecture declaration.
architecture arch_rocket_soc of rocket_soc is

  --! @name Buffered in/out signals.
  --! @details All signals that are connected with in/out pads must be passed
  --!          through the dedicated buffere modules. For FPGA they are implemented
  --!          as an empty devices but ASIC couldn't be made without buffering.
  --! @{
  signal ib_rst     : std_logic;
  signal ib_sclk_p  : std_logic;
  signal ib_sclk_n  : std_logic;
  signal ib_dip     : std_logic_vector(3 downto 0);
  signal ib_gmiiclk : std_logic;
  --! @}

  signal wSysReset  : std_ulogic; -- Internal system reset. MUST NOT USED BY DEVICES.
  signal wReset     : std_ulogic; -- Global reset active HIGH
  signal wNReset    : std_ulogic; -- Global reset active LOW
  signal soft_rst   : std_logic; -- reset from exteranl debugger
  signal bus_nrst   : std_ulogic; -- Global reset and Soft Reset active LOW
  signal wClkBus    : std_ulogic; -- bus clock from the internal PLL (100MHz virtex6/40MHz Spartan6)
  signal wPllLocked : std_ulogic; -- PLL status signal. 0=Unlocked; 1=locked.

  --! Arbiter is switching only slaves output signal, data from noc
  --! is connected to all slaves and to the arbiter itself.
  signal aximi   : nasti_master_in_type;
  signal aximo   : nasti_master_out_vector;
  signal axisi   : nasti_slave_in_type;
  signal axiso   : nasti_slaves_out_vector;
  signal slv_cfg : nasti_slave_cfg_vector;
  signal mst_cfg : nasti_master_cfg_vector;
   
  signal eth_i : eth_in_type;
  signal eth_o : eth_out_type;

 
  signal irq_pins : std_logic_vector(CFG_IRQ_TOTAL-1 downto 0);
begin

  --! PAD buffers:
  irst0   : ibuf_tech generic map(CFG_PADTECH) port map (ib_rst, i_rst);
  iclkp0  : ibuf_tech generic map(CFG_PADTECH) port map (ib_sclk_p, i_sclk_p);
  iclkn0  : ibuf_tech generic map(CFG_PADTECH) port map (ib_sclk_n, i_sclk_n);
  dipx : for i in 0 to 3 generate
     idipz  : ibuf_tech generic map(CFG_PADTECH) port map (ib_dip(i), i_dip(i));
  end generate;
  diffclk: if CFG_RMII = 0 generate 
  igbebuf0 : igdsbuf_tech generic map (CFG_PADTECH) port map (
            i_gmiiclk_p, i_gmiiclk_n, ib_gmiiclk);
  end generate;

  --! @todo all other in/out signals via buffers:

  ------------------------------------
  -- @brief Internal PLL device instance.
  pll0 : SysPLL_tech generic map (
    tech => CFG_FABTECH,
    tmode_always_ena => CFG_TESTMODE_ON
  ) port map (
    i_reset     => ib_rst,
    i_clkp	     => ib_sclk_p,
    i_clkn	     => ib_sclk_n,
    o_clk_bus   => wClkBus,
    o_locked    => wPllLocked,
    o_clk_50_quad => o_erefclk,
    o_clk_50     => eth_i.rmii_clk
  );
  wSysReset <= ib_rst or not wPllLocked;

  ------------------------------------
  --! @brief System Reset device instance.
  rst0 : reset_global port map (
    inSysReset  => wSysReset,
    inSysClk    => wClkBus,
    inPllLock   => wPllLocked,
    outReset    => wReset
  );
  wNReset <= not wReset;
  bus_nrst <= not (wReset or soft_rst);

  --! @brief AXI4 controller.
  ctrl0 : axictrl port map (
    clk    => wClkBus,
    nrst   => wNReset,
    slvoi  => axiso,
    mstoi  => aximo,
    slvio  => axisi,
    mstio  => aximi
  );
  
  ------------------------------------
  --! @brief Controller of the LEDs, DIPs and GPIO with the AXI4 interface.
  --! @details Map address:
  --!          0x80000000..0x80000fff (4 KB total)
  gpio0 : nasti_gpio generic map (
    xindex   => CFG_NASTI_SLAVE_GPIO,
    xaddr    => 16#80000#,
    xmask    => 16#fffff#
  ) port map (
    clk   => wClkBus,
    nrst  => wNReset,
    cfg   => slv_cfg(CFG_NASTI_SLAVE_GPIO),
    i     => axisi,
    o     => axiso(CFG_NASTI_SLAVE_GPIO),
    i_dip => ib_dip,
    o_led => o_led
  ); 
 
  --! @brief Ethernet MAC with the AXI4 interface.
  --! @details Map address:
  --!          0x80040000..0x8007ffff (256 KB total)
  --!          EDCL IP: 192.168.0.51 = C0.A8.00.33
  eth0_rmii_ena1 : if CFG_RMII = 1 generate 
    eth_i.rx_crs <= i_erx_dv;
  end generate;
  eth0_rmii_ena0 : if CFG_RMII = 0 generate -- plain MII
    eth_i.rx_dv <= i_erx_dv;
    eth_i.rx_crs <= i_erx_crs;
  end generate;

    eth_i.tx_clk <= i_etx_clk;
    eth_i.rx_clk <= i_erx_clk;
    eth_i.rx_er <= i_erx_er;
    eth_i.rx_col <= i_erx_col;
    eth_i.rxd <= i_erxd;
    eth_i.mdint <= i_emdint;

    mac0 : grethaxi generic map (
      xslvindex => CFG_NASTI_SLAVE_ETHMAC,
      xmstindex => CFG_NASTI_MASTER_ETHMAC,
      xaddr => 16#80040#,
      xmask => 16#FFFC0#,
      xirq => CFG_IRQ_ETHMAC,
      memtech => CFG_MEMTECH,
      mdcscaler => 50,  --! System Bus clock in MHz
      enable_mdio => 1,
      fifosize => 16,
      nsync => 1,
      edcl => 1,
      edclbufsz => 16,
      macaddrh => 16#20789#,
      macaddrl => 16#123#,
      ipaddrh => 16#C0A8#,
      ipaddrl => 16#0033#,
      phyrstadr => 7,
      enable_mdint => 1,
      maxsize => 1518,
      rmii => CFG_RMII
   ) port map (
      rst => wNReset,
      clk => wClkBus,
      msti => aximi,
      msto => aximo(CFG_NASTI_MASTER_ETHMAC),
      mstcfg => mst_cfg(CFG_NASTI_MASTER_ETHMAC),
      msto2 => open,    -- EDCL separate access is disabled
      mstcfg2 => open,  -- EDCL separate access is disabled
      slvi => axisi,
      slvo => axiso(CFG_NASTI_SLAVE_ETHMAC),
      slvcfg => slv_cfg(CFG_NASTI_SLAVE_ETHMAC),
      ethi => eth_i,
      etho => eth_o,
      irq => irq_pins(CFG_IRQ_ETHMAC)
    );
 
  emdio_pad : iobuf_tech generic map(
      CFG_PADTECH
  ) port map (
      o  => eth_i.mdio_i,
      io => io_emdio,
      i  => eth_o.mdio_o,
      t  => eth_o.mdio_oe
  );
  o_egtx_clk <= eth_i.gtx_clk;--eth_i.tx_clk_90;
  o_etxd <= eth_o.txd;
  o_etx_en <= eth_o.tx_en;
  o_etx_er <= eth_o.tx_er;
  o_emdc <= eth_o.mdc;
  o_erstn <= wNReset;

end arch_rocket_soc;
