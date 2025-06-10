# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct C:\Users\shaza\Documents\Xilinx_ZCU102\fpga_demo\sdk_v2\FPGA_Demo\platform.tcl
# 
# OR launch xsct and run below command.
# source C:\Users\shaza\Documents\Xilinx_ZCU102\fpga_demo\sdk_v2\FPGA_Demo\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {FPGA_Demo}\
-hw {C:\Users\shaza\Documents\Xilinx_ZCU102\fpga_demo\project_shell_v2\top_wrapper.xsa}\
-proc {psu_cortexr5_0} -os {standalone} -fsbl-target {psu_cortexr5_0} -out {C:/Users/shaza/Documents/Xilinx_ZCU102/fpga_demo/sdk_v2}

platform write
platform generate -domains 
platform active {FPGA_Demo}
platform pmufw -extra-compiler-flags {-MMD -MP      -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mcpu=v9.2 -mxl-soft-mul -Os -flto -ffat-lto-objects  -DFSBL_DEBUG_DETAILED -DENABLE_SCHEDULER -DXPU_INTR_DEBUG_PRINT_ENABLE -DXPFW_DEBUG_DETAILED }
platform write
platform pmufw -extra-linker-flags {}
platform write
platform fsbl -extra-compiler-flags {-MMD -MP      -Wall -fmessage-length=0 -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -DARMR5 -Os -flto -ffat-lto-objects  -DFSBL_DEBUG_DETAILED}
platform write
platform fsbl -extra-linker-flags {}
platform write
domain create -name {domain_secure_cortexr5_0} -os {standalone} -proc {psu_cortexr5_0} -arch {32-bit} -display-name {domain_secure_cortexr5_0} -desc {} -runtime {cpp}
platform generate -domains 
platform write
domain -report -json
domain create -name {domain_nonsecure_cortexa53_0} -os {standalone} -proc {psu_cortexa53_0} -arch {64-bit} -display-name {domain_nonsecure_cortexa53_0} -desc {} -runtime {cpp}
platform generate -domains 
domain -report -json
platform write
platform clean
platform generate
platform clean
platform generate
platform clean
domain active {zynqmp_pmufw}
bsp reload
bsp removelib -name xilfpga
platform generate
platform clean
platform active {FPGA_Demo}
platform generate
platform clean
platform generate
platform clean
platform generate
bsp reload
bsp removelib -name xilfpga
bsp write
bsp reload
catch {bsp regenerate}
platform clean
bsp setlib -name xilfpga -ver 6.4
bsp write
bsp reload
catch {bsp regenerate}
domain active {zynqmp_fsbl}
bsp reload
bsp reload
platform clean
platform generate
bsp reload
domain active {zynqmp_pmufw}
bsp reload
platform clean
platform generate
platform clean
platform generate
platform active {FPGA_Demo}
bsp reload
domain active {domain_secure_cortexr5_0}
bsp reload
domain active {zynqmp_fsbl}
bsp reload
domain active {domain_secure_cortexr5_0}
bsp reload
platform active {FPGA_Demo}
platform config -updatehw {C:/Crosscon_original/project_shell_v2/top_wrapper.xsa}
platform config -updatehw {C:/Crosscon_original/project_shell_v2/top_wrapper.xsa}
platform config -updatehw {C:/Crosscon_original/project_shell_v2/top_wrapper.xsa}
platform generate
platform clean
platform generate
platform clean
platform generate
platform active {FPGA_Demo}
platform active {FPGA_Demo}
platform config -updatehw {C:/Crosscon_original/project_shell_v2/top_wrapper.xsa}
platform generate
platform generate
platform active {FPGA_Demo}
platform generate -domains 
platform clean
platform generate
platform clean
platform generate
platform generate
platform active {FPGA_Demo}
platform generate
