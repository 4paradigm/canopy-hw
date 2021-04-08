Canopy: FPGA-Accelerated Deep Learning Compiler Stack (Hardware Backend)
===

## Introduction

This is the hardware backend of Canopy (https://github.com/4paradigm/canopy), it implements the versatile tensor accelerator on FPGA, which is the hardware foundation of Canopy. This repository consists of the hardware implementation of the accelerator, its PCIE drivers and necessary files for testing, emulation as well as simulation.

Canopy is an evolved version of Apache TVM developed by 4Paradigm. TVM is an open-source deep learning compiler stack that supports a variety of hardware backends, such as CPUs, GPUs, and FPGAs. However, the supported FPGA is only limited to SoC FPGAs with the shared memory space between an ARM core and FPGA. Based on such a hardware architecture, modern high-end PCIE-based cloud FPGAs cannot take advantage from TVM because of the separate memory space between the host and FPGA. Therefore, the major purpose of Canopy project is to enable the open-source deep learning compiler stack to run efficiently on PCIE-based high-end FPGAs. Compared with TVM, there are three major contributions:

- We have designed the PCIE driver to adapt to the TVM framework, which makes it possible to work with PCIE-based FPGA cards.
- We have defined and implemented the underlying FPGA-based accelerator using high-level synthesis languages OpenCL. Therefore, any hardware accelerators with OpenCL support can be integrated into Canopy with minimum code modification efforts.
- We have extended and fixed a few issues from TVM to make it support FPGA smoothly.

As a result, Canopy is able to compile and run deep learning models on PCIE-based high-end FPGAs. Our design supports both Intel and Xilinx's FPGAs, and has been tested on Intel Arria 10, Stratix 10, and AWS F1 instances (with Xilinx Virtex UltraScale+).

**Note that we have open-sourced the baseline version of Canopy. For any commercial supports, customized design, and further improvement, please contact 4Paradigm: LU Mian lumian@4paradigm.com**

## OpenCL-based Hardware Implementation

The original FPGA support from Apache TVM is only limited to SoC FPGAs with the shared memory between host and FPGA devices. In this repository, we included the following implementations suitable for cloud-based FPGAs:

- hardware/intelfocl: For Intel® OpenCL for FPGA™ compatible devices like Intel Arria® 10 GX FPGA Development Kit or Intel® Programmable Acceleration Cards
- hardware/xilinxvitis: For Xilinx® Vitis™ compatible devices like Xilinx UltraScale+ VU9P Card on Amazon Elastic Compute Cloud

The PCIE-based drivers can be found at src/oclfpga directory.

## Contribution to Apache TVM

This repository is forked from Apache TVM-VTA (https://github.com/apache/tvm-vta). We greatly appreciate the significant work done by the TVM team that has defined a neat but yet flexible design for deep learning compiler and accelerator research. Meanwhile, we have been contributing most of our work into the TVM community. To understand what we have proposed to the TVM community, please refer to the following RFC and discussion.

- https://discuss.tvm.apache.org/t/rfc-vta-support-for-cloud-devices-opencl-compatible/6676
- https://github.com/apache/tvm/issues/5840

According to the RFC, we have submitted tens of PRs to the TVM community (dated Dec 2020) and more ongoing.

- (merged) https://github.com/apache/tvm-vta/pull/7
- (merged) https://github.com/apache/tvm-vta/pull/6
- (merged) https://github.com/apache/tvm-vta/pull/9
- https://github.com/apache/tvm-vta/pull/8

**However, a more efficient hardware design as well as the Xilinx FPGA support are included only in this repository. We strongly recommend you to start your evaluation on AWS EC2 F1 instances as following, which is easy to develop, compile, and deploy your models.**

## Evaluation on Amazon EC2 F1 Instances

Please refer to Canopy (https://github.com/4paradigm/canopy) for detailed steps of evaluating this hardware accelerator on Amazon EC2 F1 Instances.

## Workshop Talks

https://www.youtube.com/watch?v=6Qg7R6EuAjE

## Development Team

Canopy is an open-source project developed by the HPC team of 4Paradigm. For any technical feedbacks, please contact the authors:
ZHANG Hao: zhanghao@4paradigm.com
LI Jiashu: lijiashu@4paradigm.com
LU Mian: lumian@4paradigm.com
