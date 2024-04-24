
# Secure FPGA Provisioning

In this repository you can find the initial version of Secure FPGA Provisioning to provide secure FPGA-based acceleration and enable IP protection on FPGA-enabled SoCs within [CROSSCON project](https://crosscon.eu/). This work is part of the deliverable 4.2 (CROSSCON Extension Primitives to Domain Specific Hardware Architectures — Initial Version) and the deliverable 3.2 (CROSSCON Open Security Stack – Initial Version). The repository 3 folders. Folder Bitstreams contains a main bitstream and 4 partial bitstreams. Each partial bitstream represents a configuration file that corresponds to a specific accelerator targeting a virtual FPGA, e.g., file vfpga_1_shift_left.bin configures a shift left accelerator on vFPGA_1. Folder figures shows the sequence of the demo. Folder sdk contains the Xilinx Vitis workspace of our demo. This workspace contains the required files and applications to replicate the demo. For detailed description of the CROSSCON SoC, please refer to the deliverable 4.1 (ROSSCON Extensions to Domain Specific Hardware Architectures Documentation — Draft) of the CROSSCON project.

## What is the Demo about?

In this demo, an FPGA shell is implemented to take care of partial configuration through internal configuration port (ICAP) port, which is internal to the FPGA fabric. In addition to the shell, two logically-isolated virtual FPGAs are implemented on the FPGA, each can be managed separately. On vFPGA_1 we can run a shift circuit (shifting right or left) and on vFPGA_2 we can run a counter circuit (counting up or down). Figure figures/design shows the block design of the FPGA shell, vFPGA_1 and vFPGA_2. It also shows the external pins of our designs: count_out and shift_out are connected to the eight PL leds. Each vFPGA is connected to 4 leds of the PL leds on the board (figures/board). The leds blinking pattern will reflect the direction (counting up or down) and (shifting right or left). The inputs on the design are connected to general-purpose switches (SW14, SW15, SW16, SW17 and SW18) to control the configuration controller manually.  
The configuration controller in the shell receives the required information from a trusted application, controlling the FPGA resources and providing FPGA services to other applications, i.e., which vFPGA and which bitstream to configure on it. 
PCAP port can be used to program the FPGA from the processing system, i.e., Arm cores, however it can be used by any application to do so. To prevent unauthorized access to FPGA logic, the PCAP is deactivated (PCAP and ICAP work exclusively). The controlling application is responsible for deactivating PCAP and enabling ICAP and configuring the FPGA and loading partial bitstreams in memory in preparation for the partial configuration process.  This represents TA_FPGA as discueed in D4.1.

## How to run it?
Due to known issue of ([ZCU102](https://support.xilinx.com/s/article/71968?language=en_US)) Evaluation Board, we are running the demo in the debug mode. Due to this limitation, we were only able to test basic functionality with the FPGA. That is, we used a user interface to interact with the controlling application rather than implementing another application that communicates with controlling application to request FPGA services. To recreate the Vitis workspace, follow these simple steps:

 1. Launch Xilinx Vitis and choose a location for your workspace. The version used in the demo is Vitis 2023.1.

<p align="center">
    <img src="./figures/step1.png" width=50% height=50%>
</p> 
<p align="center">Figure 1: Xilinx Vitis <p align="center">
 
 2. Choose "Create Application Project" and click next.

<p align="center">
    <img src="./figures/step2.png" width=50% height=50%>
</p> 
<p align="center">Figure 2: Xilinx Vitis: Create New Application Project <p align="center">
 
 3. In tab "Create a new platform from hardware (XSA)", click on Browse .. and choose the file ([Bitstreams/top.xsa](https://github.com/crosscon/FPGA_TEE/blob/main/Bitstreams/top.xsa)). This file contains the description of the entire platform including the hardware design representing the shell.

<p align="center">
    <img src="./figures/step3.png" width=50% height=50%>
</p> 
<p align="center">Figure 3: Loading Platform Description <p align="center">
 
Once loaded, keep the default settings and click next. In the field "Application project name" provide a name to the application. Make sure the application is associated with processor psu_cortexa53_0 and click next. Keep default settings and click next.

4. From "Templates" choose an empty application (c) and click finish.

<p align="center">
    <img src="./figures/step4.png" width=50% height=50%>
</p> 
<p align="center">Figure 4: Setting up a new Application from Template <p align="center">

5. In the Explorer tab, you can see the application_name [standalone_psu_cortexta53_0]. Expand it and right click on src folder. Choose from the menu import resources ... 
In field "From directory", provide the path to ([sdk/sources/](https://github.com/crosscon/FPGA_TEE/tree/main/sdk/sources)), the source files will apear in the window, select them and click finish. This application deactivates the PCAP and gives control to the application running on the FPGA shell

<p align="center">
    <img src="./figures/step5.png" width=50% height=50%>
</p> 
<p align="center">Figure 5: Adding Source Files to the Application <p align="center">
 
6. Right click on the application_name [standalone_psu_cortexta53_0] and choose Build Project.

<p align="center">
    <img src="./figures/step6.png" width=50% height=50%>
</p> 
<p align="center">Figure 6: Compiling the Application <p align="center">

7. In the main toolbar, select the small drop-menu next to the run symbol and  slect "Run Configurations.." Select "Single Application Debug" and cerate new one.

<p align="center">
    <img src="./figures/step7.png" width=50% height=50%>
</p> 
<p align="center">Figure 7: Creating Run Configurations <p align="center">

8. Keep the default values in the Main tab and move to the next tab.

<p align="center">
    <img src="./figures/step8.png" width=50% height=50%>
</p> 
<p align="center">Figure 8: Main tab - Run Configurations <p align="center">

9. In the Application tab, make sure application_name appears in the Project field. If not, browse for the generated elf file.

<p align="center">
    <img src="./figures/step9.png" width=50% height=50%>
</p> 
<p align="center">Figure 9: Application tab - Run Configurations <p align="center">
 
10. Click on the "hier_mb_mb" from the "Summary" window and leave the Project and Application fields empty. Click on Edits for more advanced options. Add the partial bitstream files ([Bitstreams/*.bin](https://github.com/crosscon/FPGA_TEE/tree/main/Bitstreams)) in the window "Data Files to download before launch" in the locations shown in the figure.

<p align="center">
    <img src="./figures/step10.png" width=50% height=50%>
</p> 
<p align="center">Figure 10: Uploading Partial Bitstreams - Run Configurations <p align="center">

 This step will upload the partial bitstreams in the memory allocated for the FPGA shell.

11. In the "Target Setup" tab, fill the "Hardware Platform" and "Bitstream File" with the files provided in  ([Bitstreams/top.xsa](https://github.com/crosscon/FPGA_TEE/blob/main/Bitstreams/top.xsa)) and ([Bitstreams/top.bit](https://github.com/crosscon/FPGA_TEE/blob/main/Bitstreams/top.bit))
In the "Summary" window, you will see the run steps.

<p align="center">
    <img src="./figures/step11.png" width=50% height=50%>
</p> 
<p align="center">Figure 11: Target Setup - Run Configurations <p align="center">

Now connect the USB and JTAG on the ZCU102 board to your machine, as shown below:

<p align="center">
    <img src="./figures/board.png" width=50% height=50%>
</p> 
<p align="center">Figure 12: ZCU102 Connections <p align="center">

Make sure SW6 is set to JTAG mode (all four switches are on). Then, open a serial terminal, COM7 with baud rate of 115200. 
Once switched on, run the application in Xilinx Vitis. Once the FPGA is configured, we can see the Menu on the terminal.
The first two options unlock the virtual FPGAs so that they can be reconfigured. In the figure below, 1 was typed in and vFPGA_1 is activated.

<p align="center">
    <img src="./figures/demo1.png" width=50% height=50%>
</p> 
<p align="center">Figure 13: Main Menu - FPGA Shell <p align="center">
 
Now we can freely configure vFPGA_1. By typing 3, shift left accelerator is loaded on vFPGA_1. 

<p align="center">
    <img src="./figures/demo2.png" width=50% height=50%>
</p> 
<p align="center">Figure 14: Configuring vFPGA_1 - FPGA Shell <p align="center">

  Typing 6 in the terminal shows that vFPGA_2 cannot be configured before being activated first.
 
<p align="center">
    <img src="./figures/demo3.png" width=50% height=50%>
</p> 
<p align="center">Figure 15: Configuring vFPGA_2 - FPGA Shell <p align="center">

Activating vFPGA_2 by typing 2 in. 

<p align="center">
    <img src="./figures/demo4.png" width=50% height=50%>
</p> 
<p align="center">Figure 16: Activating vFPGA_2 - FPGA Shell <p align="center">

 Now vFPGA_2 can be reconfigured.

<p align="center">
    <img src="./figures/board.png" width=50% height=50%>
</p> 
<p align="center">Figure 17: Exiting Demo <p align="center">

 We can freely lock, activate and switch between the different accelerators on the vFPGAs. 
Our next steps is to implement a second trusted application that will communicate with the trusted application controlling the FPGA services and implement the necessary cryptographic operations to enable partial bitstreams decryption and verification.

## Licence

See LICENCE file.

## Acknowledgments

The work presented in this repository is part of the [CROSSCON project](https://crosscon.eu/) that received funding from the European Union’s Horizon Europe research and innovation programme under grant agreement No 101070537.

<p align="center">
    <img src="https://crosscon.eu/sites/crosscon/themes/crosscon/images/eu.svg" width=10% height=10%>
</p>

<p align="center">
    <img src="https://crosscon.eu/sites/crosscon/files/public/styles/large_1080_/public/content-images/media/2023/crosscon_logo.png?itok=LUH3ejzO" width=25% height=25%>
</p>
