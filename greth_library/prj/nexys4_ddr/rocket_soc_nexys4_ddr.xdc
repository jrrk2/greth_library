## This file is a general .xdc for the Nexys4 DDR Rev. C

set_property PACKAGE_PIN E3 [get_ports i_sclk_p]
set_property IOSTANDARD LVCMOS33 [get_ports i_sclk_p]
#set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets i_sclk_p_IBUF]

create_clock -period 10.000 -name sys_clk_pin -waveform {0.000 5.000} -add [get_ports i_sclk_p]

#create_generated_clock -name eth_clk90 -source [get_pins ethclk.PLLE2_ADV_inst/CLKIN1] [get_pins ethclk.PLLE2_ADV_inst/CLKOUT1]
#create_generated_clock -name clkm -source [get_pins clkgen_gen.clkgen0/xc7l.v/PLLE2_ADV_inst/CLKIN1] [get_pins clkgen_gen.clkgen0/xc7l.v/PLLE2_ADV_inst/CLKOUT0]

## Switches

set_property PACKAGE_PIN J15 [get_ports {i_dip[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[0]}]
set_property PACKAGE_PIN L16 [get_ports {i_dip[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[1]}]
set_property PACKAGE_PIN M13 [get_ports {i_dip[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[2]}]
set_property PACKAGE_PIN R15 [get_ports {i_dip[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[3]}]
#set_property PACKAGE_PIN R17 [get_ports {i_dip[4]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[4]}]
#set_property PACKAGE_PIN T18 [get_ports {i_dip[5]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[5]}]
#set_property PACKAGE_PIN U18 [get_ports {i_dip[6]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[6]}]
#set_property PACKAGE_PIN R13 [get_ports {i_dip[7]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[7]}]
## SW8 and SW9 are in the same bank of the DDR2 interface, which requires 1.8 V
#set_property PACKAGE_PIN T8 [get_ports {i_dip[8]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {i_dip[8]}]
#set_property PACKAGE_PIN U8 [get_ports {i_dip[9]}]
#set_property IOSTANDARD LVCMOS18 [get_ports {i_dip[9]}]
#set_property PACKAGE_PIN R16 [get_ports {i_dip[10]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[10]}]
#set_property PACKAGE_PIN T13 [get_ports {i_dip[11]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[11]}]
#set_property PACKAGE_PIN H6 [get_ports {i_dip[12]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[12]}]
#set_property PACKAGE_PIN U12 [get_ports {i_dip[13]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[13]}]
#set_property PACKAGE_PIN U11 [get_ports {i_dip[14]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[14]}]
#set_property PACKAGE_PIN V10 [get_ports {i_dip[15]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_dip[15]}]


## o_leds

set_property PACKAGE_PIN H17 [get_ports {o_led[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_led[0]}]
set_property PACKAGE_PIN K15 [get_ports {o_led[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_led[1]}]
set_property PACKAGE_PIN J13 [get_ports {o_led[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_led[2]}]
set_property PACKAGE_PIN N14 [get_ports {o_led[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_led[3]}]
set_property PACKAGE_PIN R18 [get_ports {o_led[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_led[4]}]
set_property PACKAGE_PIN V17 [get_ports {o_led[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_led[5]}]
set_property PACKAGE_PIN U17 [get_ports {o_led[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_led[6]}]
set_property PACKAGE_PIN U16 [get_ports {o_led[7]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_led[7]}]
#set_property PACKAGE_PIN V16 [get_ports {o_led[8]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {o_led[8]}]
#set_property PACKAGE_PIN T15 [get_ports {o_led[9]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {o_led[9]}]
#set_property PACKAGE_PIN U14 [get_ports {o_led[10]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {o_led[10]}]
#set_property PACKAGE_PIN T16 [get_ports {o_led[11]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {o_led[11]}]
set_property PACKAGE_PIN V15 [get_ports {o_etx_er}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_etx_er}]
set_property PACKAGE_PIN V14 [get_ports {o_egtx_clk}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_egtx_clk}]
set_property PACKAGE_PIN V12 [get_ports {o_etxd[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_etxd[2]}]
set_property PACKAGE_PIN V11 [get_ports {o_etxd[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_etxd[3]}]

##Buttons

set_property PACKAGE_PIN N17 [get_ports i_rst]
set_property IOSTANDARD LVCMOS33 [get_ports i_rst]

#set_property PACKAGE_PIN P17 [get_ports {i_etxd[2]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_etxd[2]}]
#set_property PACKAGE_PIN M17 [get_ports {i_etxd[3]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {i_etxd[3]}]

#set_property PACKAGE_PIN C4 [get_ports i_uart1_rd]
#set_property IOSTANDARD LVCMOS33 [get_ports i_uart1_rd]
#set_property PACKAGE_PIN D4 [get_ports o_uart1_td]
#set_property IOSTANDARD LVCMOS33 [get_ports o_uart1_td]

##SMSC Ethernet PHY
set_property PACKAGE_PIN C9 [get_ports o_emdc]
set_property IOSTANDARD LVCMOS33 [get_ports o_emdc]
set_property PACKAGE_PIN A9 [get_ports io_emdio]
set_property IOSTANDARD LVCMOS33 [get_ports io_emdio]
set_property PACKAGE_PIN B3 [get_ports o_erstn]
set_property IOSTANDARD LVCMOS33 [get_ports o_erstn]
set_property PACKAGE_PIN D9 [get_ports i_erx_dv]
set_property IOSTANDARD LVCMOS33 [get_ports i_erx_dv]
set_property PACKAGE_PIN C10 [get_ports i_erx_er]
set_property IOSTANDARD LVCMOS33 [get_ports i_erx_er]
set_property PACKAGE_PIN C11 [get_ports {i_erxd[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {i_erxd[0]}]
set_property PACKAGE_PIN D10 [get_ports {i_erxd[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {i_erxd[1]}]
set_property PACKAGE_PIN B9 [get_ports o_etx_en]
set_property IOSTANDARD LVCMOS33 [get_ports o_etx_en]
set_property PACKAGE_PIN A10 [get_ports {o_etxd[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_etxd[0]}]
set_property PACKAGE_PIN A8 [get_ports {o_etxd[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {o_etxd[1]}]
set_property PACKAGE_PIN D5 [get_ports o_erefclk]
set_property IOSTANDARD LVCMOS33 [get_ports o_erefclk]
set_property -dict { PACKAGE_PIN B8    IOSTANDARD LVCMOS33 } [get_ports { i_emdint }]; #IO_L12P_T1_MRCC_16 Sch=eth_intn

