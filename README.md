# Secure FPGA Provisioning

In this repository, you can find the initial version of Secure FPGA Provisioning, which provides secure FPGA-based acceleration and enables IP protection on FPGA-enabled SoCs within \[CROSSCON project\](https://crosscon.eu/). This work is part of deliverable 4.2, 4,.3 (CROSSCON Extension Primitives to Domain Specific Hardware Architectures — Initia/Final Version) and deliverable 3.2/3.3 (CROSSCON Open Security Stack – Initial/Final Version). Folder Bitstreams contains a primary bitstream and four partial bitstreams. Each partial bitstream represents a configuration file corresponding to a specific accelerator targeting a virtual FPGA, e.g., file vfpga_1_shift_left_partial.bin configures a shift left accelerator on vFPGA_1. The folder figures shows the demo sequence, and the folder SDK contains the Xilinx Vitis source files to replicate the demo. For a detailed description of the Secure FPGA Provisioning demo, please refer to the deliverable 4.1 (CROSSCON Extensions to Domain Specific Hardware Architectures Documentation — Draft) of the CROSSCON project.

## **1. Prerequisites**

- Xilinx Vitis 2023.1

- A pre-configured BSP for both application projects (appx, fpga_ta)

- wolfSSL v5.7.6 source code (downloaded and extracted, or we can use it directly from the folder: wolfssl)

- Custom user_settings.h for standalone configuration (provided)

## **2. What is the Demo about?**

In this demo, an FPGA shell is implemented to take care of partial configuration through an internal configuration port (ICAP), which is internal to the FPGA fabric. In addition to the shell, two logically-isolated virtual FPGAs are implemented on the FPGA, each of which can be managed separately. On vFPGA_1, we can run a shift circuit (shifting right or left); on vFPGA_2, we can run a counter circuit (counting up or down). Figure 1 shows the block design of the FPGA shell, vFPGA_1 and vFPGA_2.

<img src="images/media/image1.png" style="width:6.26806in;height:3.62569in" />
Figure 1<br>

The figure also shows the external pins of our designs, count_out and shift_out, connected to the eight PL LEDs. Each vFPGA is connected to 4 PL LEDs on the board. The LED blinking pattern will reflect the direction (counting up or down) and (shifting right or left). The inputs on the design are connected to general-purpose switches (SW14, SW15, SW16, SW17, and SW18) to control the configuration controller manually. The configuration controller in the shell receives the required information from a trusted application, controls the FPGA resources, and provides FPGA services to other applications, i.e., which vFPGA and which bitstream to configure on it. The PCAP port can program the FPGA from the processing system, i.e., Arm cores. However, it can be used by any application to do so. To prevent unauthorized access to FPGA logic, the PCAP is deactivated (PCAP and ICAP work exclusively). The controlling application is responsible for deactivating PCAP, enabling ICAP, configuring the FPGA, and loading partial bitstreams in memory in preparation for the partial configuration process. This represents TA_FPGA, as discussed in D4.1.

## **3. Directly use the project (Recommended)**

This document outlines the steps for a standalone bare-metal project in Xilinx Vitis for Zynq UltraScale+ devices, including both APU (appx) and RPU (fpga_ta) applications. In the Repository, there are two main folders: sdk_v2 and project_shell_v2, and other folders such as wolfssl. All necessary development and integration steps will be carried out inside the sdk_v2 folder using Vitis. The project_shell_v2 folder already contains the completed hardware design and pre-generated bitstreams required for RSA integration, so no changes are needed on that side.

After downloading the project, extract both folders to your drive. Do not rename any folders, especially project_shell_v2, as this can cause path resolution issues within Vitis. Once extracted, launch Vitis and set the workspace to the sdk_v2 directory. Inside the workspace, you will find a system project named FPGA_Demo, and two application projects: appx and fpga_ta. The appx application runs on the Cortex-A53 core, while fpga_ta runs on the Cortex-R5 core. Both are configured as standalone applications with their respective platform projects.

If you encounter a "hardware not found" error, it is likely due to a mismatch in the hardware export path. To fix this, right-click the platform project, select **Update Hardware Specification**, and point it to the top_wrapper.xsa file located inside the project_shell_v2 folder (See Figures 2 and 3).

<img src="images/media/image2.png" style="width:5.0625in;height:4.95833in" />
Figure 2<br>

<img src="images/media/image3.png" style="width:6.26806in;height:2.63819in" />
Figure 3<br>

Note: If you want to create your own project, please refer to **Appendix A**.

## **3. How to run it?**

We are still working on debug mode. Former colleagues mention the problem of Xilinx (<https://support.xilinx.com/s/article/71968?language=en_US>), which means the former ZCU102 can only work in debug mode. Now we have a new ZCU102 board. The SD card model will be added later when multi-client functions are realized. Here, we still use the debug mode and use a user interface to interact with the controlling application rather than implementing another application that communicates with the controlling application to request FPGA services.

1.  The current Vitis debug configuration is set up to launch all necessary processing units for the system: the Cortex-A53 core runs the appx application, the Cortex-R5 core runs the fpga_ta application, and the PMU is initialized with the pmufw.elf firmware. The pmufw.elf file is located in the ~sdk_v2/FPGA_Demo/export/FPGA_Demo/sw/FPGA_Demo/boot/ directory within the project structure. There is no need to manually add appx.elf and fpga_ta.elf; they will automatically appear in the debug configuration. All selected processors are configured to reset before execution to ensure proper initialization and coordination between components (See Figure 4).

<img src="images/media/image4.png" style="width:6.26772in;height:4.23611in" />
Figure 4<br>

2.  Before running in debug mode, the partial bitstream should be uploaded to the system, see Figure 5. Choose appx/psu_cortexa53_0 and then click Edit in the Advanced Options. In the advanced options of the APU application launch, a single partial bitstream is specified to be downloaded to the FPGA before execution. This bitstream corresponds to the "shift left" functionality and is loaded at address 0x20000000. The directory of this partial bitstream ~project_shell_v2\bitstreams\vfpga1_shift_left_partial_icap_bs.bin.

<img src="images/media/image5.png" style="width:6.26772in;height:4.48611in" />
Figure 5<br>

3.  In the Target Setup (refer to Figure 6), the bitstream file should be manually selected. Click Browse in Bitstream File and choose the file in ~project_shell_v2\bitstreams\top3.bit.

<img src="images/media/image6.png" style="width:6.26772in;height:4.22222in" />
Figure 6<br>

4.  Now check if the necessary directories are sourced in for appx. Click and expand **appx_system -\>** Right click on **appx \[domain_nonsecure_cortexa53_0\] -\>** Click on **C/C++ Build Settings**. As shown in Figure 7, add the correct directories in ARM v8 gcc compiler. Click on Apply and rebuild **appx**.

<img src="images/media/image7.png" style="width:6.26776in;height:6.24097in" />
Figure 7<br>

5.  Check if the necessary directories are sourced in for fpga_ta. Click and expand **fpga_ta_system -\>** Right click on **fpga_ta \[domain_secure_cortexar5_0\] -\>** Click on **C/C++ Build Settings**. As shown in Figure 8, add the correct directories in the ARM v5 gcc compiler. Click on Apply and rebuild **fpga_ta**.

<img src="images/media/image8.png" style="width:6.26806in;height:6.24097in" />
Figure 8<br>

6.  Now connect the USB and JTAG on the ZCU102 board to your machine, as shown in Figure 9.

<img src="images/media/image9.png" style="width:3.45625in;height:5.05764in" />
Figure 9<br>

7.  Ensure SW6 is set to JTAG mode (all four switches are on) as shown in Figure 10.

<img src="images/media/image10.jpeg" style="width:2.88403in;height:2.73958in" />
Figure 10<br>

8.  For the serial terminal, using **PuTTY (Figure 11)** is suggested, but you can use any other terminal program of your choice.

<img src="images/media/image11.png" style="width:4.28646in;height:3.76215in" />
Figure 11<br>

9.  Then, open two serial terminals—one for appx and one for fpga_ta, both on the appropriate COM (dependent on the home device ) ports with a baud rate of 115200. If you see more than two COM ports In your Device Manager, it is useful to open a serial terminal for all of them. You can then keep the two that show output.

> **Note:** Ensure that the USB-to-UART driver for the ZCU102 board is installed. You can download it from the following link:
>
> Check your Device Manager/Ports(COM & LPT): <https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers>

10. Now we are ready to run and deploy. Make sure the board is turned on and connected. Right click on **appx** **-\> Run as -\> Run Configurations -\>** **Debugger_appx_Default_1** -\> **Run**. You should see a window as shown in Figure 12. Once completed, the output should be visible on the serial terminals.

<img src="images/media/image12.png" style="width:6.26806in;height:2.13611in" />  
Figure 12<br>

## 4. Results

This section presents the expected output on seen via the COM ports.

### 4.1 APPx

The terminal output demonstrates a manual, step-by-step secure partial reconfiguration flow between the Cortex-A53 (APPx) and Cortex-R5 (TA_FPGA) cores on the ZCU102 board. After system boot and FSBL initialization on the RPU, APPx initializes RSA key parameters and manually performs RSA encryption of the AES key. The encrypted AES key is then divided into two chunks due to transfer size limitations and printed for verification.

Next, APPx logs the memory addresses for the requested accelerator’s plain bitstream, encrypted version, and tag. AES-GCM parameters—including keys, IVs, AADs, and expected tags—are manually set and displayed. Each element (key chunks, IV, AAD) is then transferred to TA_FPGA and acknowledged as received.

An interrupt confirms successful processing, and the system reports that decryption and authentication were completed successfully.

<img src="images/media/image13.png" style="width:4.66111in;height:3.47639in" />
Figure 13<br>

<img src="images/media/image14.png" style="width:2.39028in;height:5.05903in" />
Figure 14<br>

<img src="images/media/image15.png" style="width:2.93819in;height:5.0625in" />
Figure 15<br>

### 4.2 FPGA_TA:

The terminal output from the TA_FPGA (Cortex-R5) side shows the manual, interrupt-driven decryption and partial reconfiguration process that complements the APPx execution. After the FSBL completes and the PL is configured, the TA_FPGA firmware starts and displays a menu for selecting which accelerator to load.

Upon receiving messages from APPx via inter-processor interrupts, TA_FPGA begins receiving and logging AES key chunks, IVs, AADs, and tag data. Each interrupt handler prints out the exact content of the message, providing full visibility into the system's secure handshake process. The AES key is reconstructed from the two chunks sent earlier (split due to transfer size limits), and the decrypted full key is logged in both raw and hexadecimal format.

After successful decryption and validation of the AES-GCM parameters, the encrypted bitstream is authenticated. TA_FPGA confirms the bitstream integrity and starts secure partial reconfiguration of the “Shift Left” accelerator at the specified address. Once the configuration is complete, a confirmation message is sent back to APPx.

Overall, the output confirms that TA_FPGA correctly receives all cryptographic parameters, securely decrypts the AES key, authenticates the bitstream, and completes the accelerator reconfiguration—demonstrating the full secure boot and configuration pipeline in a transparent, debug-friendly manner.

<img src="images/media/image16.png" style="width:3.55694in;height:5.90903in" />
Figure 16<br>

<img src="images/media/image17.png" style="width:4.26042in;height:5.90625in" />
Figure 17<br>

## **Appendix A**

### A.1 Creating a Project in Vitis

1. Launch Xilinx Vitis and choose a location for your workspace. The version used in the demo is Vitis 2023.1.

<img src="images/media/image18.png" style="width:6.26772in;height:3.52778in" />
Figure 18<br>

2. Choose "Create Application Project" and click Next.

<img src="images/media/image19.png" style="width:6.03178in;height:4.50429in" />
Figure 19<br>

3\. In the tab "Create a new platform from hardware (XSA)", click on Browse .. and choose the file (project_shell_v2/top_wrapper.xsa). This file contains a description of the entire platform, including the hardware design representing the shell. Choose to generate boot components on psu cortexr5_0.

<img src="images/media/image20.png" style="width:6.26772in;height:1.56944in" />
Figure 20<br>

Once loaded, keep the default settings and click Next. In the field "Application project name," provide the application name. Make sure the application is associated with the processor psu_cortexa53_0 and click next.

<img src="images/media/image21.png" style="width:4.40104in;height:3.65535in" />
Figure 21<br>

<img src="images/media/image22.png" style="width:6.26772in;height:5.22222in" />
Figure 22<br>

4\. Keep default settings and click next. From "Templates", choose an empty application (c) and click finish.<img src="images/media/image23.png" style="width:6.26772in;height:4.91667in" />
Figure 23<br>

5. In the Explorer tab, you can see the application_name. Expand it and right-click on the src folder. Choose from the menu import resources ... In the field "From directory", provide the path to (sdk_v2\appx\src), the source files will appear in the window, select them, and click finish.

<img src="images/media/image24.png" style="width:6.26806in;height:3.36597in" />
Figure 24<br>

6\. In the Project Explorer, right click on *FPGA_Demo*, choose *New*, then choose *Application Project…,* click *Next*, choose *Select a platform from the repository*, and choose FPGA_Demo.

<img src="images/media/image25.png" style="width:6.26806in;height:5.03056in" />
Figure 25<br>

7\. Choose “psu_cortexr5_0” processor as shown below. If the processor is not listed, check the “show all processors in the hardware specification” option. Use the source files provided in the (sdk_v2\fpga_ta\src) and build the project.

<img src="images/media/image26.png" style="width:4.95313in;height:4.92844in" />
Figure 26<br>

8\. Keep the default settings and click next. From "Templates", choose an empty application (c) and click finish.

<img src="images/media/image23.png" style="width:6.26772in;height:4.91667in" />
Figure 27<br>

9\. In the Explorer tab, you can see the application_name. Expand it and right-click on the src folder. Choose from the menu, import resources ... In the field "From directory", provide the path to (sdk_v2\fpga_ta\src), the source files will appear in the window, select them, and click finish.

<img src="images/media/image27.png" style="width:6.26806in;height:4.40972in" />
Figure 28<br>

### A.2 Integrating WolfSSL

To integrate the **wolfSSL v5.7.6** cryptographic library into a standalone Vitis project targeting both the APU (appx) and RPU (fpga_ta) on a Xilinx Zynq UltraScale+ platform, several manual steps must be followed to ensure compatibility with the bare-metal environment. First, download and extract the wolfSSL source archive. Then, import the necessary source files into each application project. For both appx and fpga_ta, navigate in Vitis to the source folder (e.g., appx/src/ or fpga_ta/src/) and import **all files** from the wolfssl/src/ directory and the wolfssl/wolfcrypt/src/ directory. It is important to delete all .S assembly files inside wolfssl/wolfcrypt/src/ after import, as these are not supported by the standalone ARM toolchain and will lead to errors. Additionally, within the wolfssl/wolfcrypt/src/port/ directory, delete everything **except** the xilinx/ folder and nrf51.c.

Next, integrate the custom user configuration header file by defining the preprocessor symbol WOLFSSL_USER_SETTINGS in both application projects. In Project Explorer, go to **appx_systems (fpga_ta_system)→ appx (fpga_ta)→** right click **appx (fpga_ta) → C/C++ Build → Settings → ARM v8 (for appx) or ARM v7 (for fpga_ta) → Compiler → Symbols**, and add WOLFSSL_USER_SETTINGS to the list. There is **no need to explicitly add user_settings.h to the src/ folder** of the project; it will automatically be picked up from the include paths once the wolfSSL source directory is properly added to the project.

<img src="images/media/image28.png" style="width:6.26806in;height:5.79583in" />
Figure 29<br>

<img src="images/media/image29.png" style="width:3.13892in;height:3.84231in" />
Figure 30<br>

<img src="images/media/image30.png" style="width:3.05177in;height:3.84375in" /> 
Figure 31<br>

Then, in the Project Explorer, go to **appx_systems (fpga_ta_system)→ appx (fpga_ta)→** right click **appx (fpga_ta) → C/C++ Build → Settings → Compiler → Includes**, and add the following two include paths:

- ../wolfssl

- ../wolfssl/IDE/XilinxSDK  
  
<img src="images/media/image31.png" style="width:3.33477in;height:3.82131in" />
Figure 32<br>

<img src="images/media/image32.png" style="width:2.95313in;height:3.8752in" />
Figure 33<br>

To avoid linker script errors, such as lscript.ld not found, it is sometimes necessary to explicitly specify the path to the linker script in both application projects. In Vitis, this can be done by navigating to **appx_systems (fpga_ta_system)→ appx (fpga_ta)→** right click **appx (fpga_ta) → C/C++ Build → Settings → Linker → Miscellaneous** and adding the following flag to the “Other flags” field:

-T../src/lscript.ld

If your lscript.ld file is located in a different directory, you should update the path accordingly to reflect its actual location. This step should be applied to both the appx and fpga_ta projects to ensure that the linker can locate and use the correct script during the build process. **However, be careful.** If you are not encountering a missing lscript.ld error, and the script is already being handled correctly by Vitis, manually adding the -T flag can lead to memory region redeclaration issues. This may result in duplicated memory mappings in the final link stage, which can cause runtime hangs or unpredictable behavior. Therefore, only apply this manual step if you are explicitly facing linker script path errors.

<img src="images/media/image33.png" style="width:3.09792in;height:3.73958in" />
Figure 34<br>

<img src="images/media/image34.png" style="width:2.83819in;height:3.73611in" />
Figure 35<br>

To ensure stable operation of the wolfSSL cryptographic functions in a standalone environment, appropriate stack and heap sizes must be configured for each application. In the appx project, which runs on the Cortex-A53 and handles RSA operations and other computationally intensive tasks, the stack size is set to 0x8000 (32 KB) and the heap size to 0x4000 (16 KB) in the lscript.ld file. This provides sufficient space for cryptographic computations without causing memory overflow.

<img src="images/media/image35.png" style="width:6.26806in;height:3.275in" />
Figure 36<br>

For the fpga_ta project running on the Cortex-R5, larger memory allocations are required due to its execution model and the overhead of secure tasks. Here, the stack size is increased to 0x10000 (64 KB) and the heap size to 0x8000 (32 KB). These settings help prevent stack corruption or heap exhaustion during RSA key handling, buffer-based operations, or modular arithmetic. Both configurations are applied via the Vitis GUI under **Stack and Heap Sizes**, and they should be carefully maintained in line with the expected cryptographic workload of each processing domain.

<img src="images/media/image36.png" style="width:6.26806in;height:3.27778in" />
Figure 37<br>

wolfSSL requires a source of entropy for random number generation, which is unavailable in standalone bare-metal environments. To address this, implement a custom random seed function named my_rng_seed_gen. You should place it in src/wolfcrypt/src/ for both application projects. Create a file named my_rng_seed_gen.c in appx/src/wolfcrypt/src/ and fpga_ta/src/wolfcrypt/src/, and add the following implementation:

<img src="images/media/image37.png" style="width:3.19792in;height:2.47917in" />
Figure 38<br>

```c
#include <wolfssl/wolfcrypt/types.h>

unsigned char my_rng_seed_gen(void) {

static unsigned int seed = 12345;

seed = (seed \* 1103515245 + 12345) & 0xFFFFFFFF; // Simple LCG

return (unsigned char)(seed & 0xFF);

}
```


This lightweight linear congruential generator (LCG) provides basic entropy suitable for development. Make sure this function name matches the macro defined in your user_settings.h:

```c
#define CUSTOM_RAND_GENERATE my_rng_seed_gen
```

<img src="images/media/image38.png" style="width:6.26772in;height:1.125in" />
Figure 39<br>

You may also encounter build errors due to \<sys/uio.h\> being included by default in wolfssl/ssl.h. To prevent this, open wolfssl/ssl.h and locate the line around 3531 where \#include \<sys/uio.h\> appears. Wrap this line with the following preprocessor condition:

```c
#if !defined(WOLFSSL_NO_IO)

#include \<sys/uio.h\>

#endif
```

<img src="images/media/image39.png" style="width:6.26772in;height:3.19444in" />
**Figure 40**
<br><br><br>


After these adjustments, both appx and fpga_ta will compile and link successfully with wolfSSL, supporting standalone cryptographic operations, most importantly, RSA encryption without relying on an operating system or file system.

⚠️ **Important Warning**

Do not modify the user_settings.h file manually unless you fully understand its configuration and implications. This file has been carefully prepared to match the requirements of a standalone Vitis environment, with all necessary features enabled or disabled to prevent compatibility issues. Making changes to it—such as re-enabling socket or file system features, altering cryptographic flags, or overriding platform-specific options—may lead to build failures, runtime errors, or insecure behavior. Additionally, it is essential to follow **every step in this integration process exactly as described**. Skipping or altering steps (such as incorrectly placing source files, omitting the seed generator, or misconfiguration the linker path) can result in unresolved symbols, linker errors, or improper cryptographic operation. For a stable and secure integration, use the provided user_settings.h as-is and carefully apply each instruction.

## License

See the LICENSE file.

## Acknowledgment

The work presented in this repository is part of the [CROSSCON project](https://crosscon.eu/) that received funding from the European Union’s Horizon Europe research and innovation programme under grant agreement No 101070537.

<p align="center">
    <img src="https://crosscon.eu/sites/crosscon/themes/crosscon/images/eu.svg" width=10% height=10%>
</p>

<p align="center">
    <img src="https://crosscon.eu/sites/crosscon/files/public/styles/large_1080_/public/content-images/media/2023/crosscon_logo.png?itok=LUH3ejzO" width=25% height=25%>
</p>
