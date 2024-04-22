
# Secure FPGA Provisioning

In this repository you can find the initial version of Secure FPGA Provisioning to provide secure FPGA-based acceleration and enable IP protection on FPGA-enabled SoCs within [CROSSCON project](https://crosscon.eu/). This work is part of the deliverable 4.2 (CROSSCON Extension Primitives to Domain Specific Hardware Architectures — Initial Version) and the deliverable 3.2 (CROSSCON Open Security Stack – Initial Version). The repository 3 folders. Folder Bitstreams contains a main bitstream and 4 partial bitstreams. Each partial bitstream represents a configuration file that corresponds to a specific accelerator targeting a virtual FPGA, e.g., file vfpga_1_shift_left.bin configures a shift left accelerator on vFPGA_1. Folder figures shows the sequence of the demo. Folder sdk contains the Xilinx Vitis workspace of our demo. This workspace contains the required files and applications to replicate the demo. For detailed description of the CROSSCON SoC, please refer to the deliverable 4.1 (ROSSCON Extensions to Domain Specific Hardware Architectures Documentation — Draft) of the CROSSCON project.

## What is the Demo about?

In this demo, an FPGA shell is implemented to take care of partial configuration through internal configuration port (ICAP) port, which is internal to the FPGA fabric. In addition to the shell, two logically-isolated virtual FPGAs are implemented on the FPGA, each can be managed separately. On vFPGA_1 we can run a shift circuit (shifting right or left) and on vFPGA_2 we can run a counter circuit (counting up or down). Figure figures/design shows the block design of the FPGA shell, vFPGA_1 and vFPGA_2. It also shows the external pins of our designs: count_out and shift_out are connected to the eight PL leds. Each vFPGA is connected to 4 leds of the PL leds on the board (figures/board). The leds blinking pattern will reflect the direction (counting up or down) and (shifting right or left). The inputs on the design are connected to general-purpose switches (SW14, SW15, SW16, SW17 and SW18) to control the configuration controller manually.  
The configuration controller in the shell receives the required information from a trusted application, controlling the FPGA resources and providing FPGA services to other applications, i.e., which vFPGA and which bitstream to configure on it. 
PCAP port can be used to program the FPGA from the processing system, i.e., Arm cores, however it can be used by any application to do so. To prevent unauthorized access to FPGA logic, the PCAP is deactivated (PCAP and ICAP work exclusively). The controlling application is responsible for deactivating PCAP and enabling ICAP and configuring the FPGA and loading partial bitstreams in memory in preparation for the partial configuration process.  This represents TA_FPGA as discueed in D4.1.

## How to run it?
Due to known issue of ZCU102 Evaluation Board ([link](https://support.xilinx.com/s/article/71968?language=en_US)), we are running the demo in the debug mode. Due to this limitation, we were only able to test basic functionality with the FPGA. That is, we used a user interface to interact with the controlling application rather than implementing another application that communicates with controlling application to request FPGA services. To recreate the Vitis workspace, follow these simple steps:

 1. Launch Xilinx Vitis and choose a location for your workspace. The version used in the demo is Vitis 2023.1.
 2. Choose "Create Application Project" and click next.
 3. In tab "Create a new platform from hardware (XSA)", click on Browse .. and choose the file (sdk/sources/top.xsa). This file contains the description of the entire platform including the hardware design representing the shell.  Once loaded, keep the default settings and click next.
 4. In the field "Application project name" provide a name to the application and make sure the application is associated with processor psu_cortexa53_0. Click next.
 5. Keep default settings and click next.
 6. From Templates choose an empty application (c) and click finish.
 7. In the Explorer tab, you can see the application_name [standalone_psu_cortexta53_0]. Expand it and right click on src. Choose from the menu import resources ...
 8. In field "From directory", provide the path to (sdk/sources/icap_active), the c file will apear in the window, select it and click finish.
 9. Right click on the application_name [standalone_psu_cortexta53_0] and choose Build Project.
 
To build a boot image, use the information provided in (sdk/output.bif), see (figures/demo1) and a binary file is created based on the information provided in the bif file. This image (sdk/BOOT.bin) is used to program the flash memory as shown in (figures/demo2). This image contains the partial bitstreams that will be used to configure the vFPGAs. The board is connected as shown in figures/demo0, USB and JTAG are connected to the controlling laptop that runs Xilinx Vitis application and SW6 is set to JTAG mode (all four switches are on). Then we open a serial terminal (based on the COM port) with baud rate of 115200. 
Once the flash is programmed successfully, we run the application icap_active in Xilinx Vitis (figures/demo3). 

The icap_active application deactivates the PCAP and gives control to the application running on the FPGA shell (included in the provided top.xsa file. Once the FPGA is configured, we can see the Menu on the terminal (see figures/demo4). Typing 7 (figures/demo5), we can see the status of the vFPGAs. Mode active means that vFPGA_1 is ready to be reconfigured. State full means the vFPGA is running an accelerator circuit, RM_ID refers to the reconfigurable module ID, i.e., which accelerator. Typing 6 in the terminal (figures/demo6), we shutdown the vFPGAs such that their configurations cannot be changed. If we try to load a new accelerator on a vFPGA in shutdown mode, the request is denied (figures/demo7). Therefore, we type 5 in the terminal to activate the vFPGAs (figures/demo8). Then, we can freely switch between the different accelerators on the vFPGAs (figures/demo9-demo14). 
Our next steps is to implement a second trusted application that will communicate with the trusted application controlling the FPGA services and implement the necessary cryptographic operations to enable partial bitstreams decryption and verification.

