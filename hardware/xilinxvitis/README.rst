Xilnix Vitis Setup
------------------

To compile and run VTA on Xilinx® Vitis™ unified software platform, you need to first install and configure Xilinx® Vitis™ environment on your system. Detailed installation instructions of the Xilinx® Vitis Core Development Kit can be found at installation section of `Vitis Unified Software Development Platform Documentation <https://www.xilinx.com/html_docs/xilinx2020_1/vitis_doc/vhc1571429852245.html>`_.

If you have Xilinx® Vitis™ compatible hardware accelerator card(s) installed on your system, you could compile and run the VTA design on actual hardware. However, if you do not have any compatible card available, you may still try and test VTA in software emulation or cycle-accurate simulation modes, please jump to section 'Compile VTA kernel in Emulation Mode' for more details.

Verify hardware installation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To verify your hardware installation, simple use the xbutil utility provided by Xilinx.

.. code:: bash

    $ xbutil scan
    INFO: Found total 1 card(s), 0 are usable
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    System Configuration
    OS name:        Linux
    Release:        3.10.0-1127.10.1.el7.x86_64
    Version:        #1 SMP Wed Jun 3 14:28:03 UTC 2020
    Machine:        x86_64
    Model:          HVM domU
    CPU cores:      8
    ...


To perform a simple test on your installed acceleration cards, you could use the validate option of the xbutil utility.

.. code:: bash

    $ xbutil validate
    INFO: Found 1 cards

    INFO: Validating card[0]: 
    ...


For detailed usage of xbutil command, please refer to `Vitis Unified Software Development Platform Documentation <https://www.xilinx.com/html_docs/xilinx2020_1/vitis_doc/vhc1571429852245.html>`_.

VTA Kernel Compilation
^^^^^^^^^^^^^^^^^^^^^^

To run TVM-VTA on Xilinx® Vitis™ compatible devices, firstly you need to configure the VTA target properly.

.. code:: bash

    $ cd <tvm root>/3rdparty/vta-hw/config
    $ cp xilinxvitis_sample.json vta_config.py

After updating vta_config, you need to re-compile the TVM:

.. code:: bash

    $ cd <tvm root>
    $ make

Before compiling your VTA kernel for Xilinx® Vitis™ devices, you need to make sure all the required environment variables have been set correctly. On AWS, you can run:

.. code:: bash

    $ git clone https://github.com/aws/aws-fpga.git $AWS_FPGA_REPO_DIR  
    $ cd $AWS_FPGA_REPO_DIR
    $ source vitis_setup.sh

Change your directory to hardware/xilinxvitis:

.. code:: bash

    $ cd <tvm root>/3rdparty/vta-hw/hardware/xilinxvitis

Simply enter ``make`` for hardware compilation and generate the VTA bitstream for your Xilinx® Vitis™ device. Please note this process may take hours or even days to complete.

.. code:: bash

    $ make
    ...
    ****** v++ v2020.1 (64-bit)
      **** SW Build 2902540 on Wed May 27 19:54:35 MDT 2020
        ** Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
    ...
    INFO: [v++ 60-594] Finished kernel compilation
    ...
    INFO: [v++ 60-586] Created /.../vta_vitis.xclbin
    INFO: [v++ 60-1307] Run completed. Additional information can be found in:
    ...
    INFO: [v++ 60-791] Total elapsed time: ...


If the hardware compilation is successful, the generated bitstream can be found at <tvm root>/3rdparty/vta-hw/build/hardware/xilinxvitis/<config>/vta_vitis.xclbin

Test your compiled VTA kernel
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The xilinxvitis target uses a local RPC session and you need to program your FPGA acceleration card using the correct bitstream before any calculation. To configure that, make sure the following instructions are added to your python script.

.. code:: python

    if env.TARGET in ("xilinxvitis"):
      remote = rpc.LocalSession()
      vta.program_fpga(remote, bitstream="<your bitstream path>")

You can now run VTA tutorial test scripts to test your kernel on Xilinx® Vitis™ compatible devices!

.. code:: bash

    $ python vta/tutorials/vta_get_started.py
    ...
    ...Using FPGA device: xilinx_aws-vu9p-f1_dynamic_5_0
    ...
    Successful vector add test!

Compile VTA kernel in Emulation Mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As hardware compilation takes hours or even days to compile, you can quickly verify your VTA design via software emulation mode. What's more, the running of emulation mode does not depend on actual hardware. That means you could try and test your design even without possession of an compatible Xilinx® Vitis™ acceleration card!

As we are using emulation mode provided by Xilinx® Vitis™ Platform, we will still need to configure the VTA target to "xilinxvitis".

.. code:: bash

    $ cd <tvm root>/3rdparty/vta-hw/config
    $ vim vta_config.py
    $ cd <tvm root>
    $ make

To compile you VTA design for emulation, instead of typing ``make``, you need to enter ``make emulator`` instead.

.. code:: bash

    $ cd <tvm root>/3rdparty/vta-hw/hardware/xilinxvitis
    $ make emulator
    ...
    ****** v++ v2020.1 (64-bit)
      **** SW Build 2902540 on Wed May 27 19:54:35 MDT 2020
        ** Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
    ...
    INFO: [v++ 60-585] Compiling for software emulation target
    ...
    INFO: [v++ 60-594] Finished kernel compilation
    ...
    INFO: [v++ 60-586] Created /.../vta_vitis_emu.xclbin
    INFO: [v++ 60-1307] Run completed. Additional information can be found in:
    ...
    INFO: [v++ 60-791] Total elapsed time: ...

The compiled bitstream could be found at <tvm root>/3rdparty/vta-hw/build/hardware/xilinxvitis/<config>/vta_vitis_emu.xclbin

In order to envoke the emulator, you should set environment variable XCL_EMULATION_MODE to sw_emu before running your application.

.. code:: bash

    $ XCL_EMULATION_MODE=sw_emu python vta/tutorials/vta_get_started.py
    ...
    ...Using FPGA device: xilinx:pcie-hw-em:7v3:1.0
    ...
    Successful vector add test!

Tested Boards
^^^^^^^^^^^^^

This version of VTA design has been successfully tested on the following Xilinx® Vitis™ compatible acceleration cards:

* Xilinx UltraScale+ VU9P Card on Amazon Elastic Compute Cloud (EC2) F1 instance

