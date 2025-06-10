# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: C:\Users\shaza\Documents\Xilinx_ZCU102\fpga_demo\sdk_v2\fpga_ta_system\_ide\scripts\debugger_fpga_ta-default.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source C:\Users\shaza\Documents\Xilinx_ZCU102\fpga_demo\sdk_v2\fpga_ta_system\_ide\scripts\debugger_fpga_ta-default.tcl
# 
connect -url tcp:127.0.0.1:3121
source C:/Xilinx/Vitis/2023.1/scripts/vitis/util/zynqmp_utils.tcl
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -nocase -filter {name =~"RPU*"}
enable_split_mode
targets -set -filter {jtag_cable_name =~ "Digilent JTAG-SMT2NC 210308B3B53D" && level==0 && jtag_device_ctx=="jsn-JTAG-SMT2NC-210308B3B53D-24738093-0"}
fpga -file C:/Users/shaza/Documents/Xilinx_ZCU102/fpga_demo/project_shell_v2/bitstreams/top3.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw C:/Users/shaza/Documents/Xilinx_ZCU102/fpga_demo/sdk_v2/FPGA_Demo/export/FPGA_Demo/hw/top_wrapper.xsa -mem-ranges [list {0x80000000 0xbfffffff} {0x400000000 0x5ffffffff} {0x1000000000 0x7fffffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
set mode [expr [mrd -value 0xFF5E0200] & 0xf]
targets -set -nocase -filter {name =~ "*R5*#0"}
rst -processor
dow C:/Users/shaza/Documents/Xilinx_ZCU102/fpga_demo/sdk_v2/FPGA_Demo/export/FPGA_Demo/sw/FPGA_Demo/boot/fsbl.elf
set bp_29_22_fsbl_bp [bpadd -addr &XFsbl_Exit]
con -block -timeout 60
bpremove $bp_29_22_fsbl_bp
targets -set -nocase -filter {name =~ "*R5*#0"}
rst -processor
dow -data C:/Users/shaza/Documents/Xilinx_ZCU102/fpga_demo/project_shell_v2/bitstreams/vfpga2_count_down_partial_icap_bs.bin 0x80000000
dow -data C:/Users/shaza/Documents/Xilinx_ZCU102/fpga_demo/project_shell_v2/bitstreams/vfpga2_count_up_partial_icap_bs.bin 0x80400000
dow -data C:/Users/shaza/Documents/Xilinx_ZCU102/fpga_demo/project_shell_v2/bitstreams/vfpga1_shift_left_partial_icap_bs.bin 0x80800000
dow -data C:/Users/shaza/Documents/Xilinx_ZCU102/fpga_demo/project_shell_v2/bitstreams/vfpga1_shift_right_partial_icap_bs.bin 0x80c00000
dow C:/Users/shaza/Documents/Xilinx_ZCU102/fpga_demo/sdk_v2/fpga_ta/Debug/fpga_ta.elf
configparams force-mem-access 0
targets -set -nocase -filter {name =~ "*R5*#0"}
con
