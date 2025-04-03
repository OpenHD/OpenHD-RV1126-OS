# 瑞芯微多核异构系统开发指南

文件标识：RK-KF-YF-160

发布版本：V1.1.0

日期：2023-11-02

文件密级：□绝密   □秘密   □内部资料   ■公开

**免责声明**

本文档按“现状”提供，瑞芯微电子股份有限公司（“本公司”，下同）不对本文档的任何陈述、信息和内容的准确性、可靠性、完整性、适销性、特定目的性和非侵权性提供任何明示或暗示的声明或保证。本文档仅作为使用指导的参考。

由于产品版本升级或其他原因，本文档将可能在未经任何通知的情况下，不定期进行更新或修改。

**商标声明**

“Rockchip”、“瑞芯微”、“瑞芯”均为本公司的注册商标，归本公司所有。

本文档可能提及的其他所有注册商标或商标，由其各自拥有者所有。

**版权所有 © 2023 瑞芯微电子股份有限公司**

超越合理使用范畴，非经本公司书面许可，任何单位和个人不得擅自摘抄、复制本文档内容的部分或全部，并不得以任何形式传播。

瑞芯微电子股份有限公司

Rockchip Electronics Co., Ltd.

地址：     福建省福州市铜盘路软件园A区18号

网址：     [www.rock-chips.com](http://www.rock-chips.com)

客户服务电话： +86-4007-700-590

客户服务传真： +86-591-83951833

客户服务邮箱： [fae@rock-chips.com](mailto:fae@rock-chips.com)

---

**前言**

**概述**

本文档主要指导工程师基于瑞芯微多核异构系统进行项目开发。

**平台支持**

| **芯片名称** | **AP+AP** | **AP+MCU** | **Linux** | **RTOS** | **Bare-metal** |
| :------: | :-------: | :--------: | :-------: | :------: | :------------: |
|  RK3568  |     ✔     |     ✔      |     ✔     |    ✔     |       ✔        |
|  RK3308  |     ✔     |            |     ✔     |    ✔     |       ✔        |
|  RK3358  |     ✔     |            |           |    ✔     |       ✔        |
|  RK3562  |     ✔     |     ✔      |     ✔     |    ✔     |       ✔        |

**读者对象**

本文档（本指南）主要适用于以下工程师：

技术支持工程师

软件开发工程师

**修订记录**

| **版本号** |   **作者**    |  **修改日期**  | **修改说明**                   |
| :-----: | :---------: | :--------: | :------------------------- |
| V1.0.0  | 刘诗舫、邹鸿名、杨汉兴 | 2023-05-15 | 初始版本                       |
| V1.1.0  | 郑永智、黄子晗、邹鸿名、郑嘉航 | 2023-11-02 | 增加编译配置、资源划分、通信方案、功能模块等章节内容 |

---

**目录**

[TOC]

---


## 第1章 多核异构系统

### 概述

#### 多核异构系统简介

多核异构系统是一种将同一颗 SoC 芯片中不同处理器核心分别独立运行不同平台的计算系统。同时支持 SMP (Symmetric Multi-Processing) 对称多处理系统和 AMP (Asymmetric Multi-Processing) 非对称多处理系统。

多核异构系统将传统平台两套系统合二为一。在传统平台中，Linux系统和实时性系统往往是完全独立的两套系统，需要完整的两颗处理器和两套外围电路。而在多核异构系统中，通过合理的处理器核心、外设等资源划分，同一颗 SoC 芯片能够独立运行Linux系统和实时性系统，满足系统在软件功能和硬件外设的丰富性要求的同时，满足系统的实时性要求。

多核异构系统也能够独立运行多个实时性系统，一些实时性系统支持 SMP 的运行方式。

多核异构系统应用于产品设计中，还具有明显的性价比优势和产品体积优势。目前已经广泛应用于电力、工控等行业应用和扫地机等消费级产品中。

#### 瑞芯微多核异构系统

瑞芯微多核异构系统是瑞芯微提供的一套通用多核异构系统解决方案。在现有 SMP 对称多处理系统的基础上，增加对 AMP 非对称多处理系统的支持。本文主要对 AMP 方案进行说明。

AMP 方案主要包括 AMP 启动方案和 AMP 通信方案。具体实现原理参考本文相关章节。

在运行平台方面， Linux 提供标准的 Linux Kernel ， RTOS 提供开源的 RT-Thread ， Bare-metal 提供基于 RK HAL 硬件抽象层的裸机开发库。同时，瑞芯微多核异构系统支持客户自行适配更多的运行平台，例如可以基于 RK HAL 硬件抽象层适配指定的 RTOS 等。

在处理器核心方面，瑞芯微多核异构系统支持 SoC 中同构的 ARM Cortex-A 每个处理器核心独立运行。也支持 SoC 中异构的 ARM Cortex-M 或 RISC-V 核心独立运行。瑞芯微多核异构系统通过合理的处理器核心资源划分，将适当的任务分配到最适合的核心进行处理，从而使SoC发挥出更优秀的性能和能效表现。

目前，瑞芯微多核异构系统采用无监督的 AMP 方案。不使用虚拟化管理，从而在运行实时性系统时获得更快的中断响应，以满足电力、工控等行业应用中严苛的硬实时性要求。

未来，瑞芯微多核异构系统也将基于 RPMsg 和 RemoteProc 框架，支持标准的 OpenAMP ，将虚拟化管理作为可选特性。

### 平台支持

#### RK3568

##### 处理器核心

| 处理器类型 | 处理器核心 | Cache和TCM支持情况 |
| -------- | -------- | ----------------- |
| AP       | 4 x ARM Cortex-A55 | 32KB L1 I-Cache [private per core]<br/>32KB L1 D-Cache with ECC [private per core]<br/>512KB L3-Cache with ECC [shared with 4 core] |
| MCU      | 1 x RISC-V         | N/A                                                           |

##### 平台支持情况

| 处理器类型 | 多核异构系统支持情况 |
| -------- | ----------------- |
| 4 x ARM Cortex-A55 | 4 x HAL or RTOS<br/>Linux (CPU0) + 3 x HAL or RTOS<br/>Linux (CPU0 / CPU1) + 2 x HAL or RTOS<br/>Linux (CPU0 / CPU1 / CPU2) + HAL or RTOS |
| 1 x RISC-V         | HAL or RTOS                                                           |

#### RK3308

##### 处理器核心

| 处理器类型 | 处理器核心 | Cache和TCM支持情况 |
| -------- | -------- | ----------------- |
| AP       | 4 x ARM Cortex-A35 | 32KB L1 I-Cache [private per core]<br/>32KB L1 D-Cache [private per core]<br/>256KB L2-Cache [shared with 4 core] |

##### 平台支持情况

| 处理器类型 | 多核异构系统支持情况 |
| -------- | ----------------- |
| 4 x ARM Cortex-A35 | 4 x HAL or RTOS<br/>Linux (CPU0) + 3 x HAL or RTOS<br/>Linux (CPU0 / CPU1) + 2 x HAL or RTOS<br/>Linux (CPU0 / CPU1 / CPU2) + HAL or RTOS |

#### RK3358

##### 处理器核心

| 处理器类型 | 处理器核心 | Cache和TCM支持情况 |
| -------- | -------- | ----------------- |
| AP       | 4 x ARM Cortex-A35 | 32KB L1 I-Cache [private per core]<br/>32KB L1 D-Cache [private per core]<br/>256KB L2-Cache [shared with 4 core] |

##### 平台支持情况

| 处理器类型 | 多核异构系统支持情况 |
| -------- | ----------------- |
| 4 x ARM Cortex-A35 | 4 x HAL or RTOS |

#### RK3562

##### 处理器核心

| 处理器类型 | 处理器核心 | Cache和TCM支持情况 |
| -------- | -------- | ----------------- |
| AP       | 4 x ARM Cortex-A53 | 32KB L1 I-Cache [private per core]<br/>32KB L1 D-Cache [private per core]<br/>256KB L2-Cache [shared with 4 core] |
| MCU      | 1 x ARM Cortex-M0  | 16KB                                           |

##### 平台支持情况

| 处理器类型 | 多核异构系统支持情况 |
| -------- | ----------------- |
| 4 x ARM Cortex-A53 | 4 x HAL or RTOS<br/>Linux (CPU0) + 3 x HAL or RTOS<br/>Linux (CPU0 / CPU1) + 2 x HAL or RTOS<br/>Linux (CPU0 / CPU1 / CPU2) + HAL or RTOS |
| 1 x ARM Cortex-M0 | HAL or RTOS                                                           |

### 产品案例介绍

#### 电力继电保护装置

在电力继电保护装置中，既对系统的实时性有要求，例如对各种电气量进行实时采集和数据分析、对保护控制信号进行实时响应等；又对系统的丰富性有要求，需要使用复杂的软件功能和硬件外设，例如显示设备、USB设备、以太网设备等。使用 RK3568 运行瑞芯微多核异构系统，通过合理的处理器核心、外设等资源划分，一套板卡就能同时独立运行实时性系统和 Linux 系统，实现上述所有功能。

瑞芯微多核异构系统将传统平台两套系统所需的两套板卡合二为一，具有明显的性价比优势和产品体积优势。并且在用于实时性系统处理任务时，得益于 RK3568 ARM Cortex-A55 的高性能特性，也能获得运行更高效、算力更强劲的使用体验。

#### 扫地机器人

在扫地机器人产品中，瑞芯微多核异构系统可以解决感知、路径规划和运动控制等关键任务。扫地机器人通常需要实时感知环境、确定最佳清扫路径、进行精确的运动控制等功能。使用瑞芯微多核异构系统，可以通过运行 Linux 系统进行复杂的算法处理、网络连接、地图存储等，通过运行实时性系统来进行传感器数据采集、电机控制等。

瑞芯微多核异构系统将传统平台两颗处理器协同的工作合二为一。一方面可以节省外挂实时性处理器带来的额外成本，另一方面可以有效地减少产品处理器部分硬件的体积。

#### PLC及运动控制

在 PLC (Programmable Logic Controller) 及运动控制产品中，瑞芯微多核异构系统可以用于工业自动化领域的逻辑控制和通信任务。 PLC 通常需要实时采集信号、进行逻辑控制、与其他设备进行通信等功能。使用瑞芯微多核异构系统，通过运行 Linux 系统进行复杂的标准的网络协议栈部署和设备通信，通过运行实时性系统来进行信号的采集、实时的逻辑控制等。

瑞芯微多核异构系统广泛应用于工业自动化领域，为众多客户提供性能更强、稳定可靠的行业解决方案。

---


## 第2章 AMP SDK

### 目录结构

（SDK主要目录对应说明）

### 运行平台

#### Linux

（说明主要文件：资源保护的rockchip_amp.c、核间通信的rpmsg等）

#### RTOS

（说明主要部分bsp、common driver、test。说明与HAL的关系）

#### Bare-metal

（说明主要部分lib、project、test、middleware。说明CMSIS）

#### U-Boot

（说明主要部分AMP启动流程，区分AP和MCU，区分CPU0启动和CPU3启动）

#### rkbin

（说明主要部分AMP启动流程，区分AP和MCU，区分CPU0启动和CPU3启动）

### 基础固件

（说明基础固件提供的内容，固件说明build_info.txt，分区表信息parameter.txt）

### SDK获取

（简要说明，详细说明参考每个芯片SDK release的文档）



---


## 第3章 编译配置

### 配置文件

用户在SDK编译之前需要先进行编译配置，本章节将对主要的配置文件进行说明，以方便用户能够更加清晰的了解编译配置方法。

#### SDK统一编译配置文件

##### 统一编译默认配置文件

File：SDK/device/rockchip/.chips/rkxxxx/rockchip_xxxx_defconfig

参考如下：

```shell
RK_UBOOT_CFG="rk3308"                               # U-Boot配置文件
RK_UBOOT_CFG_FRAGMENTS="rk3308-amp.config"          # U-Boot补充配置文件
RK_UBOOT_INI="RK3308MINIALL_UART4.ini"              # U-Boot INI文件
RK_UBOOT_TRUST_INI="RK3308TRUST_CPU3.ini"           # U-Boot Trust文件

RK_RTOS=y
RK_RTOS_CFG="rockchip_rk3308_rtos_linux_64bit_cfg"  # RTOS辅助配置文件
RK_RTOS_FIT_ITS="rockchip_rk3308_rtos_linux.its"    # AMP ITS文件

RK_KERNEL_DTS_NAME="rk3308b-evb-amic-v10-amp"       # Kernel DTS文件
RK_PARAMETER="parameters-rtos-linux-amp-64bit.txt"  # Flash分区参数文件
## 第3章 ......
```

##### RTOS辅助配置文件

File：SDK/device/rockchip/.chips/rkxxxx/xxxx_cfg

参考如下：

```shell
## 第3章 RT-Thread config
RK_RTOS_RTT0_BOARD_CONFIG="board/rk3308_ddr2_v10/amp_rtt0_defconfig" # CPU0 RTT默认配置文件
RK_RTOS_RTT1_BOARD_CONFIG="board/rk3308_ddr2_v10/amp_rtt1_defconfig" # CPU1 RTT默认配置文件
RK_RTOS_RTT2_BOARD_CONFIG="board/rk3308_ddr2_v10/amp_rtt2_defconfig" # CPU2 RTT默认配置文件
RK_RTOS_RTT3_BOARD_CONFIG="board/rk3308_ddr2_v10/amp_rtt3_defconfig" # CPU3 RTT默认配置文件

## 第3章 在不包含linux的AMP系统中，RT-Thread作为主核运行时，如果有rootfs系统需求，参照以下配置
## 第3章 RTT rootfs用户数据路径，实际路径SDK/rtos/bsp/rockchip/rkxxxx/userdata
RK_RTOS_RTT_ROOTFS_DATA="userdata"
## 第3章 RTT rootfs参数，文件系统制作时，需要从Flash分区中读取rootfs信息地址与大小
RK_RTOS_RTT_ROOTFS_PARAMETERS="parameters-rtos-amp.txt"

AMP_KERNEL_ENABLE=true            # linux使能环境变量，AMP系统包含linux时，配置该变量

## 第3章 Share Memory config
RTT_SHRPMSG_SIZE=0x00080000       # RTT共享内存信息
RTT_SHRAMFS_SIZE=0x00020000       # RTT共享内存信息
RTT_SHLOG0_SIZE=0x00001000        # RTT共享内存信息
RTT_SHLOG1_SIZE=0x00001000        # RTT共享内存信息
RTT_SHLOG2_SIZE=0x00001000        # RTT共享内存信息
RTT_SHLOG3_SIZE=0x00001000        # RTT共享内存信息

## 第3章 HAL config
## 第3章 Share Memory config same as RTT
SHRPMSG_SIZE=$RTT_SHRPMSG_SIZE  # HAL共享内存信息
SHRAMFS_SIZE=$RTT_SHRAMFS_SIZE  # HAL共享内存信息
SHLOG0_SIZE=$RTT_SHLOG0_SIZE    # HAL共享内存信息
SHLOG1_SIZE=$RTT_SHLOG1_SIZE    # HAL共享内存信息
SHLOG2_SIZE=$RTT_SHLOG2_SIZE    # HAL共享内存信息
SHLOG3_SIZE=$RTT_SHLOG3_SIZE    # HAL共享内存信息
```

##### ITS配置文件

File：SDK/device/rockchip/.chips/rkxxxx/xxxx.its

参考如下：

```shell
/dts-v1/;
/ {
    description = "FIT source file for rockchip AMP";
    #address-cells = <1>;

    # RTOS AMP 镜像配置，如果RTOS分配多个CPU，需要根据CPU ID定义多个“images”镜像
    # 该示例中，RTOS仅使用CPU3，对应镜像“amp3”
    images {
        amp3 {
            description  = "bare-mental-core3";
            data         = /incbin/("cpu3.bin");    # 镜像加载二进制文件
            type         = "firmware";
            compression  = "none";
            arch         = "arm";            # “arm” or “arm64”，默认arm
            sys          = "rtt";            # CPU运行系统：“rtt” or “hal”
            cpu          = <0x3>;            # CPU ID
            thumb        = <0>;
            hyp          = <0>;
            load         = <0x02e00000>;    # 该CPU镜像的SDRAM起始地址，同时也是镜像加载地址
            size         = <0x00400000>;    # 该CPU镜像的SDRAM分配大小
            srambase     = <0xfffa0000>;    # 该CPU镜像的SRAM起始地址（根据需要分配，不需要为0）
            sramsize     = <0x00010000>;    # 该CPU镜像的SRAM分配大小（根据需要分配，不需要为0）
            udelay       = <10000>;         # CPU启动延时，多个CPU依次启动时的延时时间
            hash {
                algo = "sha256";
            };
        };
    };
        # ......
    };

    # 共享内存信息
    share_memory {
        base         = <0x03200000>;        # 多核CPU共享SDRAM内存起始地址
        size         = <0x00100000>;        # 多核CPU共享SDRAM内存分配大小
    };

    configurations {
        default = "conf";
        conf {
            description = "Rockchip AMP images";
            rollback-index = <0x0>;

            # 镜像加载列表：当存在多个amp镜像时，
            # 如：loadables = “amp0”，“amp1”，“amp2”，“amp3”...
            # 当CPU0为启动核时，实际加载顺序为：amp1-->amp2-->amp3->...->amp0
            # 当CPU1为启动核时，实际加载顺序为：amp0-->amp2-->amp3->...->amp1
            # 依次类推，启动核由于在boot阶段被占用，在AMP中将是最后一个启动
            loadables = "amp3";        # 加载镜像，该示例中只有“amp3”

            # 主核ID，当多个AMP包含多个CPU时，该参数设定主CPU ID
            # 纯RTOS AMP中默认为“0x01”
            # 当AMP系统中包含linux时，linux cpu0 默认配置为主核
            primary = <0x0>;        # 该示例中，由于包含linux，主核ID为0

            # ......

            /* - run linux on cpu0
             * - it is brought up by amp(that run on U-Boot)
             * - it is boot entry depends on U-Boot
             */
            # linux AMP配置
            linux {
                description  = "linux-os";
                arch         = "arm64";      # "arm" or "arm64"
                cpu          = <0x0>;        # CPU ID
                thumb        = <0>;
                hyp          = <0>;
                udelay       = <0>;
            };
        };
    };
};
```

注意：由于AMP系统中多个CPU共同使用同一个物理内存（SDRAM or SRAM），因此各个CPU之间以及与共享内存之间的内存分配不可冲突。

##### Flash分区表配置

File：SDK/device/rockchip/.chips/rkxxxx/parameters-xxxx.txt

在开发过程中，用户经常需要根据实际Flash大小以及各个固件模块大小，调整分区，以满足存储需求。Flash分区表主要修改的地方如下：

```shell
CMDLINE:mtdparts=:0x00001000@0x00002000(uboot),0x00001000@0x00003000(trust),...,-@0x00006800(userdata:grow)
```

格式：size@addr，单位为sector（512Byte）。

#### U-Boot配置文件

File：SDK/u-boot/configs/rkxxxx_defconfig

U-Boot作为独立模块编译时，配置参考如下：

```shell
cd u-boot
make rkxxxx_defconfig                   # 选择configs/rkxxxx_defconfig默认配置文件
make menuconfig                         # 打开配置菜单配置选项
make savedefconfig                      # 保存默认配置结果
cp defconfig configs/rkxxxx_defconfig   # 保存修改后的默认配置文件
```

#### Kernel配置文件

File：SDK/kernel/arch/arm(64)/configs/rkxxxx_defconfig

Kernel作为独立模块编译时，配置参考如下：

```shell
cd kernel
make ARCH=arm64 rkxxxx_defconfig                   # 选择默认配置文件
make ARCH=arm64 menuconfig                         # 打开配置菜单配置选项
make ARCH=arm64 savedefconfig                      # 保存默认配置结果
cp defconfig arch/arm64/configs/rkxxxx_defconfig   # 保存修改后的默认配置文件
```

#### RT-Thread配置文件

File：SDK/rtos/bsp/rockchip/rkxxxx/board/xxxx_defconfig

RT-Thread作为独立模块编译时，配置参考如下：

```shell
cd SDK/rtos/bsp/rockchip/rkxxxx             # 进入RT-Thread工程目录
cp board/xxxx/xxxx_defconfig .config        # 选择RT-Thread的板级默认配置文件
scons --menuconfig                          # 打开配置菜单配置选项
cp .config board/xxxx/xxxx_defconfig        # 保存修改后的默认配置文件
cp rtconfig.h board/xxxx/xxxx_defconfig.h   # 保存修改后的默认配置文件对应的C语言头文件
```

#### HAL配置文件

File：SDK/hal/project/rkxxxx/src/hal_conf.h

HAL作为独立模块编译时，直接修改配置头文件或者源代码。

### 编译命令介绍

#### SDK统一编译命令

SDK统一编译命令实现了SDK所有模块一键编译、打包等功能。也支持各个模块的单独编译。下面是SDK统一编译命令说明：

```shell
./build.sh chip         # 选择芯片平台
./build.sh lunch        # 选择默认配置文件
./build.sh              # 编译所有，一键编译命令（编译U-Boot，kernel，RTOS，固件打包等一键生成）
./build.sh uboot        # 单独编译U-Boot
./build.sh kernel       # 单独编译kernel
./build.sh rtos         # 单独编译RTOS
./build.sh cleanall     # 清除所有
```

更多的编译命令，用户可以通过“./build.sh help”命令查看。

#### U-Boot编译命令

U-Boot作为独立模块单独编译时，参考命令如下：

```shell
cd SDK/u-boot/
make rkxxxx_defconfig rk-amp.config   #如make rk3308_defconfig rk-amp.config
./make.sh
```

#### kernel编译命令

kernel作为独立模块单独编译时，参考命令如下：

```shell
cd SDK/kernel
make ARCH=arm64 rxxxxx_defconfig
make CROSS_COMPILE="工具链路径" ARCH=arm64 xxxx.img
## 第3章 工具链路径如：SDK/prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
## 第3章 xxxx.img 中xxxx是指dts的名字，如rk3308b-mipi-display-v11.dts 则写为rk3308b-mipi-display-v11.img
```

#### RT-Thread编译命令

RT-Thread作为独立模块单独编译时，参考命令如下：

```shell
cd SDK/rtos/bsp/rockchip/rkxxxx/
./build.sh <cpu_id 0~3 or all>
```

#### HAL编译命令

RT-Thread作为独立模块单独编译时，参考命令如下：

```shell
cd SDK/hal/project/rkxxxx/GCC
./build.sh <cpu_id 0~3 or all>
```

### SDK编译

#### RK3308

RK3308 AMP SDK，默认使用统一编译脚本编译。

##### Kernel-5.10(CPU0/1/2)+RTT(CPU3)

默认配置文件：SDK/device/rockchip/.chips/rk3308/rockchip_rk3308_rtos_linux_64bit_defconfig
默认内核配置：Kernel-5.10(CPU0/1/2)+RTT(CPU3)
默认串口配置：Master Core UART4M0 and Remote Core UART1

###### 配置

U-Boot配置：默认配置。

Kernel配置：默认配置。

RT-Thread配置：

```shell
cd SDK/rtos/bsp/rockchip/rk3308-32/
cp board/rk3308_ddr2_v10/amp_rtt3_defconfig .config

## 第3章 执行配置菜单命令，配置以下选项，保存并退出
scons --menuconfig

    CONFIG_RT_CONSOLE_DEVICE_NAME="uart1"
    CONFIG_RT_CONSOLE_DEVICE_MUX=0
    CONFIG_RT_USING_UART=y
    CONFIG_RT_USING_UART1=y

cp .config board/rk3308_ddr2_v10/amp_rtt3_defconfig
cp rtconfig.h board/rk3308_ddr2_v10/amp_rtt3_defconfig.h
```

###### 编译

```shell
cd SDK/
./build.sh chip      # 选择 rk3308
./build.lunch        # 选择 rockchip_rk3308_rtos_linux_64bit_defconfig
./build.sh           # 一键编译
```

##### Kernel-5.10(CPU0/1/2)+HAL(CPU3)

默认配置文件：SDK/device/rockchip/.chips/rk3308/rockchip_rk3308_rtos_linux_64bit_defconfig
默认内核配置：Kernel-5.10(CPU0/1/2)+HAL(CPU3)
默认串口配置：Master Core UART4M0 and Remote Core UART1

###### 配置

U-Boot配置：默认配置。

Kernel配置：默认配置。

ITS文件配置：SDK/device/rockchip/.chips/rk3308/rockchip_rk3308_rtos_linux.its （its文件详细参数说明请参考章节1.1.3）

```shell
/dts-v1/;
/ {
    description = "FIT source file for rockchip AMP";
    #address-cells = <1>;

    images {
        amp3 {
            # ......
            sys = "hal";    # 修改 CPU3 运行 HAL系统
            # ......
        };
    };
    # ......
};
```

HAL配置：

```c
File: SDK/rtos/bsp/rockchip/common/hal/project/rk3308/src/main.c
#define TEST_USE_UART1M0
```

###### 编译

```shell
cd SDK/
./build.sh chip      # 选择 rk3308
./build.lunch        # 选择 rockchip_rk3308_rtos_linux_64bit_defconfig
./build.sh           # 一键编译
```

##### RTT(CPU1)+HAL(CPU0/2/3)

默认配置文件：SDK/device/rockchip/.chips/rk3308/rockchip_rk3308_rtos_amp_32bit_defconfig
默认内核配置：RTT(CPU1)+HAL(CPU0/2/3)
默认串口配置：UART4

###### 配置

U-Boot配置：默认配置。

HAL配置：默认配置。

RT-Thread配置：

```shell
cd SDK/rtos/bsp/rockchip/rk3308-32/
cp board/rk3308_ddr2_v10/amp_rtt1_defconfig .config
scons --menuconfig
cp .config board/rk3308_ddr2_v10/amp_rtt1_defconfig
cp rtconfig.h board/rk3308_ddr2_v10/amp_rtt1_defconfig.h
```

###### 编译

```shell
cd SDK/
./build.sh chip      # 选择 rk3308
./build.lunch        # 选择 rockchip_rk3308_rtos_amp_32bit_defconfig
./build.sh           # 一键编译
```

---


## 第4章 资源划分

### 资源分配

#### 内存资源

以RK3308为例，RK3308支持针对不同CPU配置不同大小的内存，用户可根据自身需求，通过修改配置文件进行配置。

```
device/rockchip/rk3308/rockchip_rk3308*_cfg
device/rockchip/rk3308/***.its
```

注意：各个CPU所占用的内存之间，以及与共享内存之间，不能有内存冲突。

##### **SRAM**内存分区配置

SRAM内存分区包括CPU0到CPU3的独占内存，可用于对SDRAM的扩展使用。

以RTOS+Bare-metal为例，SRAM内存的默认配置如下：

File: device/rockchip/rk3308/rockchip_rk3308_rtos_amp.its

```
        amp0 {
            # ......
            srambase     = <0xfffb0000>;	# CPU0 SRAM起始地址
            sramsize     = <0x00010000>;	# CPU0 SRAM分配大小
            # ......
        };

        amp1 {
            # ......
            srambase     = <0xfff88000>;	# CPU1 SRAM起始地址
            sramsize     = <0x00008000>;	# CPU1 SRAM分配大小
            # ......
        };

        amp2 {
            # ......
            srambase     = <0xfff90000>;	# CPU2 SRAM起始地址
            sramsize     = <0x00010000>;	# CPU2 SRAM分配大小
            # ......
        };

        amp3 {
            # ......
            srambase     = <0xfffa0000>;	# CPU3 SRAM起始地址
            sramsize     = <0x00010000>;	# CPU3 SRAM分配大小
            # ......
        };
```

以Linux+RTOS/Bare-metal为例，SRAM内存的默认配置如下：

File: device/rockchip/rk3308/rockchip_rk3308_rtos_linux.its

```
/dts-v1/;
/ {
    description = "FIT source file for rockchip AMP";
    #address-cells = <1>;

    images {
        amp3 {
            # ......
            sys          = "rtt";           # 系统："rtt" or "hal"
            cpu          = <0x3>;           # CPU ID
            # ......
            srambase     = <0>;             # SRAM起始地址（根据需要分配，不需要为0）
            sramsize     = <0>;             # SRAM分配大小（根据需要分配，不需要为0）
            # ......
        };
    };
        # ......
    };

    # ......
};
```

##### **SDRAM** 内存分区配置

SDRAM内存分区包括CPU0到CPU3的独占内存，以及CPU0到CPU3共享内存部分，用户根据不同的CPU使用情况分配其大小。

以RTOS+Bare-metal为例，SDRAM内存的默认配置如下：

File: device/rockchip/rk3308/rockchip_rk3308_rtos_amp.its

```
        amp0 {
            # ......
            load         = <0x02600000>;	# CPU0 SDRAM起始地址
            size         = <0x00900000>;	# CPU0 SDRAM分配大小
            # ......
        };

        amp1 {
            # ......
            load         = <0x00800000>;	# CPU1 SDRAM起始地址
            size         = <0x00a00000>;	# CPU1 SDRAM分配大小
            # ......
        };

        amp2 {
            # ......
            load         = <0x01200000>;	# CPU2 SDRAM起始地址
            size         = <0x00a00000>;	# CPU2 SDRAM分配大小
            # ......
        };

        amp3 {
            # ......
            load         = <0x01c00000>;	# CPU3 SDRAM起始地址
            size         = <0x00a00000>;	# CPU3 SDRAM分配大小
            # ......
        };

    # ......
        share_memory {
		base         = <0x02f00000>;        # 共享SDRAM内存起始地址
		size         = <0x00100000>;        # 共享SDRAM内存分配大小
	};

    # ......
```

以Linux+RTOS/Bare-metal为例，SDRAM内存的默认配置如下：

File: device/rockchip/rk3308/rockchip_rk3308_rtos_linux.its

```
/dts-v1/;
/ {
    description = "FIT source file for rockchip AMP";
    #address-cells = <1>;

    images {
        amp3 {
            # ......
            sys          = "rtt";           # 系统："rtt" or "hal"
            cpu          = <0x3>;           # CPU ID
            # ......
            load         = <0x02e00000>;    # SDRAM起始地址
            size         = <0x00400000>;    # SDRAM分配大小
            # ......
        };
    };
        # ......
    };

    # 共享内存信息
    share_memory {
        base         = <0x03200000>;        # 共享SDRAM内存起始地址
        size         = <0x00100000>;        # 共享SDRAM内存分配大小
    };

	# ......
};
```

在该示例中，RTOS的内核配置为CPU3，运行RT-Thread系统。RTOS(CPU3)内存分配起始地址为0x02e00000。Kernel+RTT(HAL)的共享内存起始地址0x03200000，紧接在RTOS(CPU3)的内存分配空间之后。RTOS的内存与共享内存，都必须分配在Kernel预留的空间之内。

##### 共享内存分区配置

共享内存为SDRAM中CPU0到CPU3独占内存之外的部分，按照上述SDRAM内存分区所定义，起始地址为SHMEM_BASE，大小为SHMEM_SIZE。

共享内存用于CPU0到CPU3之间的通信、数据共享等使用。CPU0到CPU3都可访问该区间数据。SRAM内存的默认配置如下：

File：device/rockchip/rk3308/rockchip_rk3308_rtos_amp_32bit_cfg

```
## 第4章 Share Memory config
  RTT_SHRPMSG_SIZE=0x00080000
  RTT_SHRAMFS_SIZE=0x00020000
  RTT_SHLOG0_SIZE=0x00001000
  RTT_SHLOG1_SIZE=0x00001000
  RTT_SHLOG2_SIZE=0x00001000
  RTT_SHLOG3_SIZE=0x00001000

## 第4章 HAL config
## 第4章 Share memory config same as RTOS
  SHRPMSG_SIZE=$RTT_SHRPMSG_SIZE
  SHRAMFS_SIZE=$RTT_SHRAMFS_SIZE
  SHLOG0_SIZE=$RTT_SHLOG0_SIZE
  SHLOG1_SIZE=$RTT_SHLOG1_SIZE
  SHLOG2_SIZE=$RTT_SHLOG2_SIZE
  SHLOG3_SIZE=$RTT_SHLOG3_SIZE
```

#### Cache和MMU配置

以RK3308为例，分别介绍不同系统下的Cache和MMU配置。

##### Kernel

/*to-do*/

##### HAL

HAL程序中增加自定义内存，需要在hal/lib/CMSIS/Device/RK3308/Source/Templates/mmu_rk3308.c

中配置对应内存属性。默认配置如下：

```
/*
* Define MMU flat-map regions and attributes
*
*/
// Define dram address space
#if defined(NC_MEM_BASE) && defined(NC_MEM_SIZE)
    MMU_TTSection(MMUTable, FIRMWARE_BASE, (DRAM_SIZE - NC_MEM_SIZE) >> 20, Sect_Normal);
    MMU_TTSection(MMUTable, NC_MEM_BASE, NC_MEM_SIZE >> 20, Sect_Normal_NC);
#else
    MMU_TTSection(MMUTable, FIRMWARE_BASE, DRAM_SIZE >> 20, Sect_Normal);
#endif
    MMU_TTSection(MMUTable, SHMEM_BASE, SHMEM_SIZE >> 20, Sect_Normal_SH);

    //--------------------- PERIPHERALS -------------------
    MMU_TTSection(MMUTable, 0xFF000000, 15U, Sect_Device_RW);

    //--------------------- INTERNAL SRAM -----------------
    MMU_TTSection(MMUTable, 0xFFF00000, 1, Sect_Normal);

```

HAL中使用CMSIS标准方案，内存属性参考hal/lib/CMSIS/Core_A/Include/core_ca.h

##### RT-Thread

RT-Thread程序中增加自定义内存，需要在rtos/bsp/rockchip/rk3568-32/board/common/board_base.c

中配置对应内存属性。默认配置如下：

```
struct mem_desc platform_mem_desc[] =
{
#ifdef RT_USING_UNCACHE_HEA
    {FIRMWARE_BASE, FIRMWARE_BASE + FIRMWARE_SIZE - 1, FIRMWARE_BASE, NORMAL_MEM},
    {RT_UNCACHE_HEAP_BASE, RT_UNCACHE_HEAP_BASE + RT_UNCACHE_HEAP_SIZE - 1, RT_UNCACHE_HEAP_BASE, UNCACHED_MEM},
#else
    {FIRMWARE_BASE, FIRMWARE_BASE + DRAM_SIZE - 1, FIRMWARE_BASE, NORMAL_MEM},
#endif
    {SHMEM_BASE, SHMEM_BASE + SHMEM_SIZE - 1, SHMEM_BASE, SHARED_MEM},
#ifdef LINUX_RPMSG_BASE
    {LINUX_RPMSG_BASE, LINUX_RPMSG_BASE + LINUX_RPMSG_SIZE - 1, LINUX_RPMSG_BASE, UNCACHED_MEM},
#endif
    {0xFF000000, 0xFFF00000 - 1, 0xFF000000, DEVICE_MEM}, /* DEVICE */
    {0xFFF00000, 0xFFFFFFFF,     0xFFF00000, NORMAL_MEM} /* SRAM */
};
```

#### 中断资源

（说明主核做完整初始化，从核做配置覆盖）

#### 外设资源

（说明软件需要自行划分外设，说明未来的硬件资源划分）

### 资源保护

在Linux + RTOS/Bare-metal模式下，不同系统间会存在资源的竞争。所以在RTOS/Bare-metal中使用到的的一些外设、时钟等资源时需要在kernel/arch/arm(64)/rockchip/rkxxx-amp.dtsi文件中保护一下，避免和Linux那边的资源冲突。

以RK3308 的uart1为例，在Linux + RTOS/Bare-metal模式下，默认Linux系统运行的核心为主核，RTOS/Bare-metal为从核。

#### 时钟资源

从核这边的系统使用uart1定时器时，需要在Linux系统中进行时钟相关配置。

File：SDK/ernel/arch/arm64/rockchip/rk3308b-amp.dtsi

```
/ {
	rockchip_amp: rockchip-amp {
		compatible = "rockchip,amp";

        # 将uart1等使用到的外设的时钟资源分配给从核端的系统
        clocks = <&cru SCLK_UART1>, <&cru PCLK_UART1>,
                 <&cru PCLK_TIMER>, <&cru SCLK_TIMER4>, <&cru SCLK_TIMER5>;

        #参考rk308.dtsi中的uart1节点配置uart1的pinctrl属性
        pinctrl-names = "default";
        pinctrl-0 = <&uart1_xfer>;
        status = "okay";

    # ......

```

#### 引脚资源

（说明Linux中的保护）

#### 电源域资源

当运行Linux + RTOS/Bare-metal的AMP模式时，可以根据项目开发的需求自定义Linux端和 HAL/RT-Thread端各占核心的数量。以Linux端占CPU0到CPU2三个核心，HAL/RT-Thread端占据CPU3一个核心为例，配置电源域资源。

File：SDK/kernel/arch/arm64/boot/dts/rockchip/rk3308b-amp.dtsi

```
## 第4章 ......
## 第4章 关闭Linux CPU3的节点，使Linux启动时不会运行在CPU3上
&cpu3 {
	status = "disabled";
};
```


---


## 第5章 启动方案

### AP启动方案

SDK 支持 AMP 混合架构设计，使得不同的 CPU 可以运行不同的系统，以满足灵活的产品设计需求。目前支持 RTT、Linux、HAL的混合结构模型，允许这三种系统相互组合或者独立运行，以RK3562为例：

#### U-Boot 阶段

如果有使用到Linux，在U-Boot中需要指定加载 Linux 内核镜像的内存起始地址，用于启动 Linux 内核，需要依据实际RAM情况来进行配置，如下为RK3562 配置示例：

```c
diff --git a/include/configs/rk3562_common.h b/include/configs/rk3562_common.h
index b076f90d6b..4e0b333357 100644
--- a/include/configs/rk3562_common.h
+++ b/include/configs/rk3562_common.h
@@ -62,8 +62,8 @@
    "scriptaddr=0x00c00000\0" \
    "pxefile_addr_r=0x00e00000\0" \
    "fdt_addr_r=0x08300000\0" \
-	"kernel_addr_r=0x00400000\0" \
-	"kernel_addr_c=0x04080000\0" \
+	"kernel_addr_r=0x02000000\0" \
+	"kernel_addr_c=0x04880000\0" \
    "ramdisk_addr_r=0x0a200000\0"

 #include <config_distro_bootcmd.h>
--
2.38.0
```

#### Linux + HAL

AMP 混合架构设计支持Linux + HAL 的模式，在一般情况下CPU0~CPU2运行Linux系统，CPU3运行HAL

| **系统** | **CPU**        | **说明**        |
| -------- | -------------- | --------------- |
| Linux    | CPU0 CPU1 CPU2 | 执行 Linux 系统 |
| HAL      | CPU3           | 执行裸核系统    |

#### Linux + RTOS

AMP 混合架构设计支持Linux + RTOS 的模式，在一般情况下CPU0~CPU2运行Linux系统，CPU3运行RTOS
| **系统**  | **CPU**        | **说明**            |
| --------- | -------------- | ------------------- |
| Linux     | CPU0 CPU1 CPU2 | 执行 Linux 系统     |
| RT-Thread | CPU3           | 执行 RT-Thread 系统 |

#### RTOS + HAL

AMP 混合架构设计支持RTOS + HAL 的模式，组合较为灵活，可以运行如RTOS + 3 \* HAL、2 \* RTOS + 2 \* HAL等

| **系统** | **CPU** | **说明**            |
| -------- | ------- | ------------------- |
| RTT      | CPU1    | 执行 RT-Thread 系统 |
| HAL      | CPU2    | 执行裸核系统        |
| HAL      | CPU3    | 执行裸核系统        |
| HAL      | CPU0    | 执行裸核系统        |

#### 4 \* RTOS

4.1.x分支的RT-Thread同时支持AMP和SMP，及4个核同时跑4个RTOS或者只跑一个RTOS

| **系统** | **CPU** | **说明**            |
| -------- | ------- | ------------------- |
| RTT      | CPU1    | 执行 RT-Thread 系统 |
| RTT      | CPU2    | 执行 RT-Thread 系统 |
| RTT      | CPU3    | 执行 RT-Thread 系统 |
| RTT      | CPU0    | 执行 RT-Thread 系统 |

如果需要SMP系统，可以添加如下配置

```c
CONFIG_RT_USING_SMP=Y
CONFIG_RT_CPUS_NR=4
```

ITS可以参考如下配置：

```c
/dts-v1/;
/ {
	description = "FIT source file for rockchip AMP";
	#address-cells = <1>;

	images {

		amp0 {
			description  = "bare-mental-core0";
			data         = /incbin/("rtt0.bin");
			type         = "firmware";
			compression  = "none";
			arch         = "arm";	 // "arm64" or "arm"
			cpu          = <0x000>;  // mpidr
			thumb        = <0>;      // 0: arm or thumb2; 1: thumb
			hyp          = <0>;      // 0: el1/svc; 1: el2/hyp
			load         = <0x02600000>;
			udelay       = <10000>;
			hash {
				algo = "sha256";
			};
		};

	};

	configurations {
		default = "conf";
		conf {
			description = "Rockchip AMP images";
			rollback-index = <0x0>;
			loadables = "amp0";

			signature {
				algo = "sha256,rsa2048";
				padding = "pss";
				key-name-hint = "dev";
				sign-images = "loadables";
			};
		};
	};
};
```

#### 4 \* HAL

AMP 系统可以支持所有核都运行在HAL上

| **系统** | **CPU** | **说明**     |
| -------- | ------- | ------------ |
| HAL      | CPU1    | 执行裸核系统 |
| HAL      | CPU2    | 执行裸核系统 |
| HAL      | CPU3    | 执行裸核系统 |
| HAL      | CPU0    | 执行裸核系统 |

### MCU启动方案

利用AMP特性，将AP作为主要核心 ，MCU作为辅助核心跑裸核系统，辅助AMP系统实现快速响应和控制，在启动MCU时需要为其分配内存空间，以RK3562 MCU为例介绍。

#### U-Boot阶段启动

在MCU启动时需要在uboot中为MCU指定对应的RAM运行大小和起始地址，如Cache映射范围，MCU启动地址，SDK发布时会提供一套默认的配置，以下为RK3562的uboot配置：

```c
int fit_standalone_release(char *id, uintptr_t entry_point)
{
	/* bus m0 configuration: */
	/* open hclk_dcache / hclk_icache / clk_bus m0 rtc / fclk_bus_m0_core */
	writel(0x03180000, TOP_CRU_BASE + TOP_CRU_GATE_CON23);

	/* open bus m0 sclk / bus m0 hclk / bus m0 dclk */
	writel(0x00070000, TOP_CRU_BASE + TOP_CRU_CM0_GATEMASK);

	/* mcu_cache_peripheral_addr */
	writel(0xfc000000, SYS_GRF_BASE + SYS_GRF_SOC_CON5);
	writel(0xffb40000, SYS_GRF_BASE + SYS_GRF_SOC_CON6);

	sip_smc_mcu_config(ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
			   ROCKCHIP_SIP_CONFIG_MCU_CODE_START_ADDR,
			   0xffff0000 | (entry_point >> 16));
	/* 0x07c00000 is mapped to 0xa0000000 and used as shared memory for rpmsg */
	sip_smc_mcu_config(ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
			   ROCKCHIP_SIP_CONFIG_MCU_EXPERI_START_ADDR, 0xffff07c0);

	/* release dcache / icache / bus m0 jtag / bus m0 */
	writel(0x03280000, TOP_CRU_BASE + TOP_CRU_SOFTRST_CON23);

	/* release pmu m0 jtag / pmu m0 */
	/* writel(0x00050000, PMU1_CRU_BASE + PMU1_CRU_SOFTRST_CON02); */

	return 0;
}
```

#### MCU启动阶段

MCU的RAM启动地址可以通过ITS配置去指定，在uboot中会进行解析，以下为RK3562 MCU的ITS配置示例：

```c
/dts-v1/;
/ {
	description = "Rockchip AMP FIT Image";
	#address-cells = <1>;

	images {
		mcu {
			description  = "mcu";
			data         = /incbin/("./mcu.bin");
			type         = "standalone";	// must be "standalone"
			compression  = "none";
			arch         = "arm";		// "arm64" or "arm", the same as U-Boot state
			load         = <0x08200000>; //MCU 程序RAM启动地址
			udelay       = <1000000>;    //启动延时时间
			hash {
				algo = "sha256";
			};
		};
	};

	configurations {
		default = "conf";
		conf {
			description = "Rockchip AMP images";
			rollback-index = <0x0>;
			loadables = "mcu";

			signature {
				algo = "sha256,rsa2048";
				padding = "pss";
				key-name-hint = "dev";
				sign-images = "loadables";
			};
		};
	};
};
```

### AMP固件打包

SDK 发布时会提供一套默认的打包编译方式，一般在对应的工程目录下(project/rkXXX/mkImage.sh)。

使用者也可以自行编写自定义的打包方式：

1. 将 mkimage_for_windows\usr\bin 的路径添加至系统环境变量的 Path 变量中（打包工具由Rockchip提供，一般在SDK的tools/windows/RKImageMaker目录下）

2. 将编译生成的TestDemo.bin文件和 同级目录下的amp.its 文件拷贝和 mkimage.exe 放到同级目录下

3. 在 windows 下执行 mkimage.exe -f amp.its -E -p 0xe00 amp.img 命令就会在同级目录下生成 amp.img

AMP 固件打包需要依赖ITS 配置文件通过打包工具进行打包，主要需要配置**不同CPU的RAM加载地址以及启动延迟时间**，RAM的加载地址需要与内存资源划分相匹配，由于AMP 组合的灵活性，以下我们以RK3562为例介绍配置：

4\*HAL或者4\*RTOS的ITS配置如下：

```c
/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

/dts-v1/;
/ {
	description = "FIT source file for rockchip AMP";
	#address-cells = <1>;

	images {

		amp0 {
			description  = "bare-mental-core0";
			data         = /incbin/("hal0.bin");
			type         = "firmware";
			compression  = "none";
			arch         = "arm";	 // "arm64" or "arm"
			cpu          = <0x0>;    // mpidr
			thumb        = <0>;      // 0: arm or thumb2; 1: thumb
			hyp          = <0>;      // 0: el1/svc; 1: el2/hyp
			load         = <0x02000000>;
			udelay       = <10000>;
			hash {
				algo = "sha256";
			};
		};

		amp1 {
			description  = "bare-mental-core1";
			data         = /incbin/("hal1.bin");
			type         = "firmware";
			compression  = "none";
			arch         = "arm";
			cpu          = <0x1>;
			thumb        = <0>;
			hyp          = <0>;
			load         = <0x00800000>;
			udelay       = <10000>;
			hash {
				algo = "sha256";
			};
		};

		amp2 {
			description  = "bare-mental-core2";
			data         = /incbin/("hal2.bin");
			type         = "firmware";
			compression  = "none";
			arch         = "arm";
			cpu          = <0x2>;
			thumb        = <0>;
			hyp          = <0>;
			load         = <0x01000000>;
			udelay       = <10000>;
			hash {
				algo = "sha256";
			};
		};

		amp3 {
			description  = "bare-mental-core3";
			data         = /incbin/("hal3.bin");
			type         = "firmware";
			compression  = "none";
			arch         = "arm";
			cpu          = <0x3>;
			thumb        = <0>;
			hyp          = <0>;
			load         = <0x01800000>;
			udelay       = <10000>;
			hash {
				algo = "sha256";
			};
		};
	};

	configurations {
		default = "conf";
		conf {
			description = "Rockchip AMP images";
			rollback-index = <0x0>;
			loadables = "amp0", "amp1", "amp2", "amp3";

			signature {
				algo = "sha256,rsa2048";
				padding = "pss";
				key-name-hint = "dev";
				sign-images = "loadables";
			};
		};
	};
};
```

Linux + HAL 或者 Linux + RTOS 示例 ：

```c
/dts-v1/;
/ {
	description = "FIT source file for rockchip AMP";
	#address-cells = <1>;

	images {
		amp3 {
			description  = "bare-mental-core3";
			data         = /incbin/("rtt3.bin");
			type         = "firmware";
			compression  = "none";
			arch         = "arm";
			cpu          = <0x3>;
			thumb        = <0>;
			hyp          = <0>;
			load         = <0x01800000>;
			udelay       = <10000>;
			hash {
				algo = "sha256";
			};
		};

	};

	configurations {
		default = "conf";
		conf {
			description = "Rockchip AMP images";
			rollback-index = <0x0>;
			loadables = "amp3";

			signature {
				algo = "sha256,rsa2048";
				padding = "pss";
				key-name-hint = "dev";
				sign-images = "loadables";
			};

			/* - run linux on cpu0
			 * - it is brought up by amp(that run on U-Boot)
			 * - it is boot entry depends on U-Boot
			 */
			linux {
				description  = "linux-os";
				arch         = "arm64";
				cpu          = <0x000>;
				thumb        = <0>;
				hyp          = <0>;
				udelay       = <0>;
			};
		};
	};
};
```

---


## 第6章 通信方案

### 核间中断触发

瑞芯微AMP通信方案提供三种核间中断触发方式，分别是Mailbox中断触发、软件中断触发及SGI触发。另外，瑞芯微多核异构系统通常还会提供hardware spinlock来进行可靠的原子操作。

#### Mailbox中断触发

使用RK Mailbox模块进行核间通信，在触发Mailbox中断的同时，可以传输一个32 bit的Command寄存器数据和一个32 bit的Data寄存器数据。这是一种兼容异构处理器的通用核间中断触发方式。

#### 软件中断触发

使用GIC SPI中断，即共享外设中断中的reserved irq，通过主动Send Pending 触发。

#### SGI触发

使用GIC SGI，即软中断触发。由于Linux SMP占用了8个non-secure SGI中断号，而另外8个secure的SGI中断号需要特殊申请。因此，SGI触发的方式常用于多个从核进行同步。

### 底层接口方案

瑞芯微多核异构系统开放核间中断+Shared Memory底层驱动接口给客户，对于已经在使用多核异构系统的客户，可以直接替换相应底层驱动接口，完成平台移植工作。

目前核间中断触发方式支持mailbox、软件中断，共享内存Linux与仅支持uncache。

Linux下mailbox中断的方式参考如下路径的代码：drivers/rpmsg/rockchip_rpmsg_mbox.c。

Linux下软件中断的方式参考如下路径的代码：drivers/rpmsg/rockchip_rpmsg_softirq.c。

### RPMsg协议方案

#### 标准框架

瑞芯微多核异构系统提供RPMsg协议标准框架方案。支持RPMsg Name Service功能。Linux Kernel适配RPMsg，RTOS和Bare-metal适配RPMsg-Lite。 从如下框图可以看到，RPMsg也是由Master Core和Remote Core的核间中断，以及vring0、vring1、vdev buffer三段Shared Memory构成。

![CH06-rpmsg-framework](resources/CH06-rpmsg-framework.png)

#### 通信流程

主核发送时，主核从vring0中取得一块buffer，再将消息按照RPMsg协议填充。消息通过队列发送到vring1中，触发从核中断，通知从核有消息待处理。从核根据队列从vring1中取得对应的buffer，并将消息传递给注册的endpoint callback。完成消息传递后，释放使用的buffer，并等待下一笔数据发送。从核发送时，则与主核发送流程相反。通信过程中的共享数据放在vdev buffer中。以下两个图片为主核发送和从核发送的流程框图。

![rpmsg-communication-process1](resources/CH6-rpmsg-communication-process1.png)

![rpmsg-communication-process2](resources/CH6-rpmsg-communication-process2.png)

#### Linux Kernel适配RPMsg

##### 代码结构

Linux Kerne RPMsg主要代码结构如下图。

![rpmsg-linux-framework](resources/CH06-rpmsg-linux-framework.png)

rockchip_rpmsg_mbox.c是注册在Platform Bus上的driver，同时向VirtIO Bus注册device。它是基于mailbox核间中断+Shared Memory底层驱动接口实现的物理层（Physical Layer）。

rockchip_rpmsg_softirq.c也是注册在Platform Bus上的driver，同时向VirtIO Bus注册device。它是基于softirq核间中断+Shared Memory底层驱动接口实现的物理层（Physical Layer）。

virtio_rpmsg_bus.c是注册在VirtIO Bus上的driver，同时向RPMsg Bus注册device。VirtIO和Virtqueue是通用RPMsg协议选择的MAC层（MAC Layer）。

rpmsg_core.c则是创建RPMsg Bus，并提供传输层（Transport Layer）接口。

rockchip_rpmsg_test.c提供一个简单的核间通信通道创建和数据收发的示例。

##### RTOS端适配RPMsg-Lite

RTOS RPMsg主要代码结构如下图。

![rpmsg-rtos-framework](resources/CH06-rpmsg-rtos-framework.png)

RPMsg-Lite由恩智浦（NXP）提供，结构与Linux RPMsg类似。RPMsg-Lite提供不同RTOS的支持，即框图中的rpmsg_env_xxx.c。瑞芯微多核异构系统适配了Bare-metal和RT-Thread，客户可以直接使用。当然，也可以参考这部分代码，实现对指定RTOS的支持。

### RPMsg测试示例

#### RK3308

##### kernel+rtt

RK3308使用标准的Kernel RPMSG核间通信框架，底层适配使用VirtIO方案。主要代码路径如下：

```shell
kernel/drivers/rpmsg/rpmsg_core.c
kernel/drivers/rpmsg/virtio_rpmsg_bus.c
kernel/drivers/rpmsg/rockchip_rpmsg_softirq.c
kernel/include/linux/rpmsg/rockchip_rpmsg.h
```

###### 共享内存

以SDK提供的demo为例，划分5M共享内存给RPMSG，其中4M为VRING BUFFER，1M为VDEV BUFFER。目前共享内存仅支持uncache。

Kernel Path: SDK/kernel/arch/arm64/boot/dts/rockchip/rk3308b-amp.dtsi

```c
reserved-memory {
		#address-cells = <2>;
		#size-cells = <2>;
		ranges;

		/* remote amp core address */
		amp_reserved: amp@2e00000 {
			reg = <0x0 0x2e00000 0x0 0x1200000>;
			no-map;
		};

		rpmsg_reserved: rpmsg@7c00000 {
			reg = <0x0 0x07c00000 0x0 0x400000>;
			no-map;
		};

		rpmsg_dma_reserved: rpmsg-dma@8000000 {
			compatible = "shared-dma-pool";
			reg = <0x0 0x08000000 0x0 0x100000>;
			no-map;
		};
	};
```

RTT Path: SDK/rtos/bsp/rockchip/rk3308-32/board/common/board_base.c

```c
1.MMU映射为uncache

    {LINUX_SHMEM_BASE, LINUX_SHMEM_BASE + LINUX_SHMEM_SIZE - 1, LINUX_SHMEM_BASE, UNCACHED_MEM},
```

###### 测试demo

####### Kernel Demo

在kernel工程中修改配置文件kernel/arch/arm64/configs/rk3308_linux_defconfig。

配置菜单配置:

```shell
make ARCH=arm64 rk3308_linux_defconfig
make ARCH=arm64 menuconfig
    # 打开以下宏开关
	CONFIG_RPMSG_ROCKCHIP_TEST
make ARCH=arm64 savedefconfig
cp defconfig arch/arm64/configs/rk3308_linux_defconfig
```

Kernel Demo Path：kernel/drivers/rpmsg/rockchip_rpmsg_test.c

```c
1.demo主要流程
static struct rpmsg_driver rockchip_rpmsg_test = {
    .drv.name   = KBUILD_MODNAME,
    .drv.owner  = THIS_MODULE,
    .id_table   = rockchip_rpmsg_test_id_table,
    .probe      = rockchip_rpmsg_test_probe,
    .callback   = rockchip_rpmsg_test_cb,
    .remove     = rockchip_rpmsg_test_remove,
};

2.rockchip_rpmsg_test_id_table
/* 等待从核announce完声明一个新的ept name,如果和下面链表中的name对应则进入probe函数中 */
static struct rpmsg_device_id rockchip_rpmsg_test_id_table[] = {
    { .name	= "rpmsg-ap3-ch0" },
    { .name = "rpmsg-mcu0-test" },
    { /* sentinel */ },
};

3.rockchip_rpmsg_test_probe
static int rockchip_rpmsg_test_probe(struct rpmsg_device *rp)
{
    int ret, size;
    uint32_t master_ept_id, remote_ept_id;
    struct instance_data *idata;

    master_ept_id = rp->src;
    remote_ept_id = rp->dst;
    dev_info(&rp->dev, "new channel: 0x%x -> 0x%x!\n", master_ept_id,
             remote_ept_id);

    /*probe发一笔数据过去给remote,让remote知道master ept id*/
    ret = rpmsg_send(rp->ept, LINUX_TEST_MSG_1, strlen(LINUX_TEST_MSG_1));
    if (ret) {
        dev_err(&rp->dev, "rpmsg_send failed: %d\n", ret);
        return ret;
    }

    /*运行测试*/
    ret = rpmsg_sendto(rp->ept, LINUX_TEST_MSG_2, strlen(LINUX_TEST_MSG_2),
                       remote_ept_id);
    if (ret) {
        dev_err(&rp->dev, "rpmsg_send failed: %d\n", ret);
        return ret;
    }

    return 0;
}

4.rockchip_rpmsg_test_cb
static int rockchip_rpmsg_test_cb(struct rpmsg_device *rp, void *payload,
                                  int payload_len, void *priv, u32 src)
{
    int ret, size;
    uint32_t remote_ept_id;
    struct instance_data *idata = dev_get_drvdata(&rp->dev);

    /* master发完一笔数据给remote后，remote也会发一笔数据过来 */
    remote_ept_id = src;
    dev_info(&rp->dev, "rx msg %s rx_count %d(remote_ept_id: 0x%x)\n",
            (char *)payload, ++idata->rx_count, remote_ept_id);

    /* 测试来回收发10000后退出 */
    if (idata->rx_count >= MSG_LIMIT) {
        dev_info(&rp->dev, "Rockchip rpmsg test exit!\n");
        return 0;
    }

    /* 收到数据后再次发送一个数据给对端 */
    ret = rpmsg_sendto(rp->ept, LINUX_TEST_MSG_2, strlen(LINUX_TEST_MSG_2),
                       remote_ept_id);
    if (ret)
        dev_err(&rp->dev, "rpmsg_send failed: %d\n", ret);
        return ret;
    }
```

具体接口函数说明如下：

| **函数**       | **说明**                                |
| -------------- | --------------------------------------- |
| rpmsg_send()   | 向远程处理器发送消息                    |
| rpmsg_sendto() | 向远程处理器发送消息，指定remote ept id |

注意：在主从核发送消息的函数中均设置了dst和src参数表示master ept id和remote ept id，对于master端来说dst和src代表 master ept id和remote ept id ，对于remote端来说dst和src代表 remote ept id和master ept id。

####### RTT Demo

配置菜单配置：scons --menuconfig

```shell
CONFIG_RT_USING_RPMSG_LITE=y
CONFIG_RT_USING_LINUX_RPMSG=y
CONFIG_RT_USING_COMMON_TEST_LINUX_RPMSG_LITE=y
```

RTT Demo Path：rtos/bsp/rockchip/common/tests/rpmsg_test.c

```c
static void rpmsg_linux_test(void)
{
    int j;
    uint32_t master_id, remote_id;
    struct rpmsg_info_t *info;
    struct rpmsg_block_t *block;
    rpmsg_queue_handle remote_queue;
    char *rx_msg = (char *)rt_malloc(RL_BUFFER_PAYLOAD_SIZE);
    uint32_t master_ept_id;
    uint32_t ept_flags;
    void *ns_cb_data;

    rpmsg_share_mem_check();
    master_id = MASTER_ID;
    remote_id = HAL_CPU_TOPOLOGY_GetCurrentCpuId();
    rt_kprintf("rpmsg remote: remote core cpu_id-%ld\n", remote_id);
    rt_kprintf("rpmsg remote: shmem_base-0x%lx shmem_end-%lx\n",
    RPMSG_LINUX_MEM_BASE, RPMSG_LINUX_MEM_END);

    info = malloc(sizeof(struct rpmsg_info_t));
    if (info == NULL) {
        rt_kprintf("info malloc error!\n");
        while (1) {
            ;
        }
    }
    info->private = malloc(sizeof(struct rpmsg_block_t));
    if (info->private == NULL) {
        rt_kprintf("info malloc error!\n");
        while (1) {
            ;
        }
    }

	/*初始化rpmsg ept*/
    info->instance = rpmsg_lite_remote_init((void *)RPMSG_LINUX_MEM_BASE,
    RL_PLATFORM_SET_LINK_ID(master_id, remote_id), RL_NO_FLAGS);
    rpmsg_lite_wait_for_link_up(info->instance);
    rt_kprintf("rpmsg remote: link up! link_id-0x%lx\n",
               info->instance->link_id);
    rpmsg_ns_bind(info->instance, rpmsg_ns_cb, &ns_cb_data);
    remote_queue  = rpmsg_queue_create(info->instance);
    info->ept = rpmsg_lite_create_ept(info->instance,
    RPMSG_RTT_REMOTE_TEST3_EPT_ID, rpmsg_queue_rx_cb, remote_queue);

    /*从核announce完声明一个新的ept name与master端对应*/
    ept_flags = RL_NS_CREATE;
    rpmsg_ns_announce(info->instance, info->ept,
    RPMSG_RTT_REMOTE_TEST_EPT3_NAME, ept_flags);

    /****************** rpmsg test run **************/
    for (j = 0; j < 100; j++)
    {
        rpmsg_queue_recv(info->instance, remote_queue,
                         (uint32_t *)&master_ept_id, rx_msg,
                         RL_BUFFER_PAYLOAD_SIZE, RL_NULL, RL_BLOCK);
    //    rpmsg_queue_recv_nocopy(remote_rpmsg, remote_queue, (uint32_t *)&src,
                                  (char **)&rx_msg, RL_NULL, RL_BLOCK);
        rt_kprintf("rpmsg remote: master_ept_id-0x%lx rx_msg: %s\n",
                   master_ept_id, rx_msg);
        rpmsg_lite_send(info->instance, info->ept, master_ept_id,
        RPMSG_RTT_TEST_MSG, strlen(RPMSG_RTT_TEST_MSG), RL_BLOCK);
    }
}
```

具体接口函数说明如下：

| **函数**                 | **说明**                                                     |
| ------------------------ | ------------------------------------------------------------ |
| rpmsg_lite_remote_init() | RPMsg-lite remote 端初始化                                   |
| rpmsg_queue_create()     | RPMsg-lite创建队列                                           |
| rpmsg_lite_create_ept()  | 创建端点                                                     |
| rpmsg_queue_recv()       | 接收到的数据自动复制到缓存区                                 |
| rpmsg_ns_bind()          | 绑定name service ept（0x35这个ept id是专门给name service用于传新通道的名字） |
| rpmsg_ns_announce()      | 声明remote new ept name                                      |
| rpmsg_lite_send()        | 发送消息                                                     |

####### 测试成功log

Linux master core RPMSG成功挂载能看到如下打印：

```shell
[    1.105178] rockchip-rpmsg 7c00000.rpmsg: rockchip rpmsg platform probe.
[    1.105228] rockchip-rpmsg 7c00000.rpmsg: assigned reserved memory node rpmsg_dma@8000000
[    1.105239] rockchip-rpmsg 7c00000.rpmsg: rpdev vdev0: vring0 0x7c00000, vring1 0x7c08000
[    1.105720] virtio_rpmsg_bus virtio0: rpmsg host is online
```

remote core发起name service announce后，Linux master core能看到如下打印：

```shell
[    1.105808] virtio_rpmsg_bus virtio0: creating channel rpmsg-ap3-ch0 addr 0xc3
[    1.105980] rockchip_rpmsg_test virtio0.rpmsg-ap3-ch0.-1.195: rpmsg master: new channel: 0x400 -> 0xc3!
```

其中，rpmsg-ap3-ch0为ept name，0x400为master ept id，0xc3为 remote ept id。

RT-Thread测试结果，开机log信息如下：

```shell
[(3)0.101.712] rpmsg remote: remote core cpu_id-3
[(3)0.101.890] rpmsg remote: shmem_base-0x7c00000 shmem_end-8100000
[(3)0.506.840] rpmsg remote: link up! link_id-0x3
```

RPMSG FLAG定义如下

```c
/* rpmsg flag bit definition
 * bit 0: Set 1 to indicate remote processor is ready
 * bit 1: Set 1 to use reserved memory region as shared DMA pool
 * bit 2: Set 1 to use cached share memory as vring buffer
 */
#define RPMSG_REMOTE_IS_READY			BIT(0)
#define RPMSG_SHARED_DMA_POOL			BIT(1)
#define RPMSG_CACHED_VRING				BIT(2)
```

##### RTT+HAL

RTOS 的 RPMsg-lite 多核通信是建立在核间中断和共享内存的基础上。通过标准化的框架，实现多核之间的通信。默认配置CPU 1为master，其他CPU为remote。

###### 共享内存

RTT共享内存开始的地址及大小

Path；

/* to-do*/

共享内存区域具体分配

Path：rtos/bsp/rockchip/rk3308-32/gcc_arm.ld.S

```c
.share_lock (NOLOAD):
    {
        . = ALIGN(64);
        PROVIDE(__spinlock_mem_start__ = .);
        . += __SPINLOCK_MEM_SIZE;
        PROVIDE(__spinlock_mem_end__ = .);
        . = ALIGN(64);
    } > SHMEM

   .share_rpmsg (NOLOAD):
    {
        . = ALIGN(0x1000);
        PROVIDE(__share_rpmsg_start__ = .);
        . += __SHARE_RPMSG_SIZE;
        PROVIDE(__share_rpmsg_end__ = .);
        . = ALIGN(0x1000);
    } > SHMEM

    .share_data :
    {
        . = ALIGN(64);
        PROVIDE(__share_data_start__ = .);
        KEEP(*(.share_data))
        PROVIDE(__share_data_end__ = .);
        . = ALIGN(64);
    } > SHMEM AT > DRAM

```

HAL共享内存开始的地址及大小

Path：

/* to-do*/

共享内存区域具体分配

Path：hal/project/rk3308/GCC/gcc_arm.ld.S

```c
.share_lock (NOLOAD) :
     {
         . = ALIGN(64);
         PROVIDE(__spinlock_mem_start__ = .);
         . += __SPINLOCK_MEM_SIZE;
         PROVIDE(__spinlock_mem_end__ = .);
         . = ALIGN(64);
     } > SHMEM

     .share_rpmsg (NOLOAD):
     {
         . = ALIGN(0x1000);
         PROVIDE(__share_rpmsg_start__ = .);
         . += SHRPMSG_SIZE;
         PROVIDE(__share_rpmsg_end__ = .);
         . = ALIGN(0x1000);
     } > SHMEM

     .share_ramfs (NOLOAD):
     {
         . = ALIGN(0x1000);
         PROVIDE(__share_ramfs_start__ = .);
         . += SHRAMFS_SIZE;
         PROVIDE(__share_ramfs_end__ = .);
         . = ALIGN(0x1000);
     } > SHMEM

     .share_log (NOLOAD):
     {
         . = ALIGN(64);
         PROVIDE(__share_log0_start__ = .);
         . += SHLOG0_SIZE;
         PROVIDE(__share_log0_end__ = .);

         . = ALIGN(64);
         PROVIDE(__share_log1_start__ = .);
         . += SHLOG1_SIZE;
         PROVIDE(__share_log1_end__ = .);

         . = ALIGN(64);
         PROVIDE(__share_log2_start__ = .);
         . += SHLOG2_SIZE;
         PROVIDE(__share_log2_end__ = .);

         . = ALIGN(64);
         PROVIDE(__share_log3_start__ = .);
         . += SHLOG3_SIZE;
         PROVIDE(__share_log4_end__ = .);
         . = ALIGN(64);
     } > SHMEM
```

###### 测试demo

####### RTT Demo

Path: SDK/rtos/bsp/rockchip/rk3308-32

配置菜单配置：scons --menuconfig

```shell
CONFIG_RT_USING_RPMSG_LITE=y
CONFIG_RT_USING_COMMON_TEST_RPMSG_LITE=y
```

RTT Demo Path：rtos/bsp/rockchip/common/tests/rpmsg_test.c

RPMsg-lite 的核心代码位于：rtos/bsp/rockchip/common/drivers/rpmsg-lite目录下。其具体接口函数说明如下：

| **函数**                      | **说明**                               |
| ----------------------------- | -------------------------------------- |
| rpmsg_lite_master_init()      | RPMsg-lite master 端初始化             |
| rpmsg_lite_remote_init()      | RPMsg-lite remote 端初始化             |
| rpmsg_lite_wait_for_link_up() | RPMsg-lite remote 端等待初始化链接成功 |
| rpmsg_queue_create()          | RPMsg-lite创建队列                     |
| rpmsg_lite_create_ept()       | 创建端点                               |
| rpmsg_queue_recv()            | 接收到的数据复制到本地buffer           |
| rpmsg_queue_recv_nocopy()     | 接收到的数据直接传递指针               |
| rpmsg_lite_send()             | 发送消息                               |

####### HAL Demo

File: SDK/hal/project/rk3308/src/main.c

```c
#define TEST_DEMO
#define TEST_USE_RPMSG_INIT
```

File: SDK/hal/project/rk3308/src/test_demo.c

```c
#define RPMSG_TEST
```

HAL Demo Path：hal/project/rk3568/src/test_demo.c

RPMsg-lite 的核心代码位于：hal/middleware/rpmsg-lite/ 目录下。其具体接口函数说明如下：

| **函数**                      | **说明**                               |
| ----------------------------- | -------------------------------------- |
| rpmsg_lite_master_init()      | RPMsg-lite master 端初始化             |
| rpmsg_lite_remote_init()      | RPMsg-lite remote 端初始化             |
| rpmsg_lite_wait_for_link_up() | RPMsg-lite remote 端等待初始化链接成功 |
| rpmsg_lite_create_ept()       | 创建端点                               |
| rpmsg_lite_send()             | 发送消息                               |

####### 测试结果

在串口终端输入串口命令查看log信息。

```shell
## 第6章 RPMSG 测试命令
msh >rpmsg_master_test

## 第6章 测试结果
[(1)21.952.622] rpmsg probe remote cpu(0) ept(0x80008000) sucess!
[(1)22.031.235] rpmsg probe remote cpu(2) ept(0x80008002) sucess!
[(1)22.086.368] rpmsg probe remote cpu(3) ept(0x80008003) sucess!
[(1)22.086.410] rpmsg_master_send: master[1]-->remote[0], remote ept addr = 0x80008000
[(0)22.152.780]rpmsg_remote_recv: remote[0]<--master[1], master ept addr = 0x80000000
[(0)22.152.959]rpmsg_remote_send: remote[0]-->master[1], master ept addr = 0x80000000
[(1)22.153.616] rpmsg_master_recv: master[1]<--remote[0], remote ept addr = 0x80008000
[(1)22.154.272] rpmsg_master_send: master[1]-->remote[2], remote ept addr = 0x80008002
[(2)22.231.397]rpmsg_remote_recv: remote[2]<--master[1], master ept addr = 0x80000002
[(2)22.231.580]rpmsg_remote_send: remote[2]-->master[1], master ept addr = 0x80000002
[(1)22.232.237] rpmsg_master_recv: master[1]<--remote[2], remote ept addr = 0x80008002
[(1)22.232.893] rpmsg_master_send: master[1]-->remote[3], remote ept addr = 0x80008003
[(3)22.286.525]rpmsg_remote_recv: remote[3]<--master[1], master ept addr = 0x80000003
[(3)22.286.706]rpmsg_remote_send: remote[3]-->master[1], master ept addr = 0x80000003
[(1)22.287.363] rpmsg_master_recv: master[1]<--remote[3], remote ept addr = 0x80008003
[(1)22.288.017] rpmsg test OK!
```

---


## 第7章 中断

### ARM GIC v2

RK3308 ⽤的 GIC400，⾛的是 GICv2 的接⼝，⽀持优先级抢占和 CPU route。对于对于 GICv2 来说，有三种中断：

1. SGI: 中断号 0-15，这是软件产⽣的中断，每个 CPU 私有。
2. PPI: 中断号 16-31，这是私有的外设中断，也是每个 CPU 私有。
3. SPI: 中断号 32-1019, 这是所有 CPU 共享的外设中断。

#### HAL 中断使用示例

中断使用步骤包含以下几个方面：

1. GIC 中断配置：配置指定中断号对应的中断优先级，以及中断服务程序由哪个 CPU 来运行。
2. GIC 中断服务程序注册：注册指定中断号对应的中断服务程序。
3. GIC 中断使能：使能中断。
4. 模块中断的配置与使能：每个模块有独立的中断配置与使能，并且因模块的功能设计不同，其配置使用方法也不尽相同。具体使用方法请参考对应的模块代码。

下面以 GPIO0 和 TIMER0 为例，来简单介绍 RK3308 HAL 中断的使用方法。

#### HAL GIC中断配置表

在该 SDK 中，GIC 的中断配置位于 hal/project/rk3308/src/main.c 文件中。参考如下：

```c
## 第7章 自定义 GIC 中断信息表
static struct GIC_AMP_IRQ_INIT_CFG irqsConfig[] = {

    # The priority higher than 0x80 is non-secure interrupt.

#ifdef AMP_LINUX_ENABLE
    GIC_AMP_IRQ_CFG_ROUTE(RPMSG_03_IRQn, 0xd0, CPU_GET_AFFINITY(3, 0)),
#if defined(TEST_USE_UART1M0)
    GIC_AMP_IRQ_CFG_ROUTE(UART1_IRQn, 0xd0, CPU_GET_AFFINITY(3, 0)),
#endif
#else // #ifdef AMP_LINUX_ENABLE
    GIC_AMP_IRQ_CFG_ROUTE(AMP0_IRQn, 0xd0, CPU_GET_AFFINITY(0, 0)),

    #......

    # GIC 中断配置结束标志，不能删，并且所有有效配置都要放在这个配置前⾯
    GIC_AMP_IRQ_CFG_ROUTE(0, 0, CPU_GET_AFFINITY(DEFAULT_IRQ_CPU, 0)),   /* sentinel */
};

## 第7章 默认 GIC 中断信息表
static struct GIC_IRQ_AMP_CTRL irqConfig = {
    #默认使用DEFAULT_IRQ_CPU来初始化 GIC 配置
    .cpuAff = CPU_GET_AFFINITY(DEFAULT_IRQ_CPU, 0),
    # 默认中断优先级
    .defPrio = 0xd0,
    # 默认使用DEFAULT_IRQ_CPU来处理中断服务程序
    .defRouteAff = CPU_GET_AFFINITY(DEFAULT_IRQ_CPU, 0),
    # 用户自定义 GIC 中断信息配置表。通过该表更改以上默认配
    .irqsCfg = &irqsConfig[0],
};

void main(void)
{
    #......

    # GIC 中断初始化。
    HAL_GIC_Init(&irqConfig);
    #......
}
```

在以上的示例中，irqConfig 为初始化默认的中断信息配置表。当用户没有自定义中断信息时，系统将按照该表的规则进行中断处理。如果用户需要修改中断配置信息，可在 irqsConfig[] 结构体中进行修改。

GIC_AMP_IRQ_CFG_ROUTE(p1, p2, p3) 参数说明如下：

| **参数** | **说明** |
| -------- | -------- |
| p1       | 中断号   |
| p2       | 优先级   |
| p3       | route    |

CPU_GET_AFFINITY(p1, p2) 参数说明如下：

| **参数** | **说明**   | **备注**              |
| -------- | ---------- | --------------------- |
| p1       | cpu_id     | 分别对应cpu 0~3       |
| p2       | cluster_id | 对于 RK3308，始终为 0 |

#### HAL GPIO中断示例

下面以 GPIO0 的 C4 pin 脚为例，简单说明 GPIO 中断使用方法。

```c
## 第7章 GPIO0 中断服务程序总入口
static void gpio_isr(int vector, void *param)
{
    #......
    HAL_GPIO_IRQHandler(GPIO0, GPIO_BANK0);
    #......
}

## 第7章 GPIO0 C4 pin 脚中断回调函数
static HAL_Status c4_call_back(eGPIO_bankId bank, uint32_t pin, void *args)
{
    #......
    return HAL_OK;
}

## 第7章 GPIO脚中断使用示例
static void gpio_test(void)
{
    #......

    # 设置 GPIO0 C4 为输入口
    HAL_GPIO_SetPinDirection(GPIO0, GPIO_PIN_C4, GPIO_IN);

    # 设置 GIC (GPIO0) 中断服务程序并使能中断
    HAL_IRQ_HANDLER_SetIRQHandler(GPIO0_IRQn, gpio_isr, NULL);
    HAL_GIC_Enable(GPIO0_IRQn);

    # 设置 GPIO0 C4 中断类型、回调函数，并且使能 GPIO0 C4 的 IO 中断
    HAL_IRQ_HANDLER_SetGpioIRQHandler(GPIO_BANK0, GPIO_PIN_C4, c4_call_back, NULL);
    HAL_GPIO_SetIntType(GPIO0, GPIO_PIN_C4, GPIO_INT_TYPE_EDGE_RISING);
    HAL_GPIO_EnableIRQ(GPIO0, GPIO_PIN_C4);
}
```

在该示例中，gpio_isr() 为 GPIO0 的 GIC 中断服务程序，该函数中通过 HAL_GPIO_IRQHandler() 来回调 GPIO0 C4 pin对应的回调函数 c4_call_back() 来处理 GPIO0 C4 pin脚所产生的中断服务程序。

#### HAL TIMER中断示例

下面以 TIMER0 为例，简单介绍 TIMER 中断的使用方法。

```c
static void timer_isr(int vector, void *param)
{
    #......
}

static void timer_test(void)
{
    #......

    # 设置 GIC (TIMER0) 中断服务程序并使能中断
    HAL_IRQ_HANDLER_SetIRQHandler(TIMER0_IRQn, timer_isr, NULL);
    HAL_GIC_Enable(TIMER0_IRQn);

    # 设置 TIMER0 配置信息，并以中断方式启动 TIMER0
    HAL_TIMER_Init(TIMER0, TIMER_FREE_RUNNING);
    HAL_TIMER_SetCount(TIMER0, 24000000);
    HAL_TIMER_Start_IT(TIMER0);
}
```

#### HAL 软中断使用方法

在 RK3308 中，不但可以通过硬件进行中断触发，也可以通过软件的方式进行中断触发。

通过软件进行中断触发之前，首先需要参考前面小结用例的介绍，进行相应的中断配置以及初始化工作。在用户需要进行软件触发的位置，通过调用以下接口函数，触发软件中断。

```shell
HAL_GIC_SetPending(IRQn); // IRQn：要触发的中断号
```

#### HAL 中断用例小结

在以上的几个示例中，我们简单的介绍了 RK3308 AMP HAL 中断的使用方法，总结如下：

1. 在 main.c 系统初始化时，通过调用 HAL_GIC_Init(&irqConfig) 函数进行中断信息的初始化配置。其中包含对 GPIO0、TIMER0 的中断信息初始化配置。
2. 在 gpio_test()、timer_test() 等中断测试用例中，分别通过 HAL_IRQ_HANDLER_SetIRQHandler() 注册对应的 GIC 中断服务程序，并通过 HAL_GIC_Enable() 函数使能对应的中断。
3. 根据 GPIO 和 TIMER 等不同的硬件模块，调用其对应的接口进行硬件、中断等信息配置，并使能对应的**模块中断**。

### ARM GIC v3

### ARM NVIC

### RISC-V中断控制器



---


## 第8章 模块

(模块章节按照：简介、示例芯片、运行平台、测试、详细文档等部分编写，字母排序)

### UART

#### RK3308

##### U-Boot

File: SDK/device/rockchip/.chips/rk3308/rockchip_rk3308_xxx_defconfig

```shell
RK_UBOOT_INI="RK3308MINIALL.ini"        # UART2
or
RK_UBOOT_INI="RK3308MINIALL_UART4.ini"  # UART4
```

##### Kernel

File: SDK/kernel/arch/arm64/boot/dts/rockchip/rk3308-evb.dts

```shell
## 第8章 UART2
&uart2 {
	pinctrl-names = "default";
	pinctrl-0 = <&uart2m0_xfer>;
	status = "okay";
};
or
## 第8章 UART4
&uart4 {
	pinctrl-names = "default";
	pinctrl-0 = <&uart4_xfer>;
	status = "okay";
};
```

File: SDK/kernel/arch/arm64/boot/dts/rockchip/rk3308b-evb-v10.dtsi

```shell
## 第8章 UART2
&fiq_debugger {
	rockchip,serial-id = <2>;
	status = "okay";
};
or
## 第8章 UART4
&fiq_debugger {
	rockchip,serial-id = <4>;
	status = "okay";
};
```

##### RT-Thread

Path: SDK/rtos/bsp/rockchip/rk3308-32

配置菜单配置：scons --menuconfig

```shell
## 第8章 UART2
CONFIG_RT_CONSOLE_DEVICE_NAME="uart2"
CONFIG_RT_USING_UART=y
CONFIG_RT_USING_UART2=y
or
## 第8章 UART4
CONFIG_RT_CONSOLE_DEVICE_NAME="uart4"
CONFIG_RT_USING_UART=y
CONFIG_RT_USING_UART4=y
```

##### HAL

File: SDK/hal/project/rk3308/src/main.c

```shell
## 第8章 UART2
static struct UART_REG *pUart = UART4;
or
## 第8章 UART4
static struct UART_REG *pUart = UART2;
```

### EMMC

#### RK3308

##### Kernel

kernel默认配置为SFC Flash。如果用户需要在kernel下使用eMMC Flash，按照以下方式修改：

File: SDK/arch/arm64/boot/dts/rockchip/rk3308b-evb-v10.dtsi

```shell
## 第8章 ......
&emmc {
	# ......
	status = "okay";	# 打开EMMC
};

## 第8章 ......
&sfc {
	status = "disabled";	# 关闭 SFC
};
```

注意：kernel下使用eMMC Flash时，在固件升级时需要选择“rootfs”选项，参考如下：

![update](resources/CH08-eMMC-update.png)

##### RT-Thread

RT-Thread默认配置关闭eMMC Flash。如果用户需要在RT-Thread下使用eMMC Flash，按照以下方式修改：

Path: SDK/rtos/bsp/rockchip/rk3308-32

配置菜单配置：scons --menuconfig

###### RT-Thread配置

eMMC Flash配置:

```shell
CONFIG_RT_USING_SDIO=y
CONFIG_RT_SDCARD_MOUNT_POINT="/"
CONFIG_RT_USING_SDIO0=y

CONFIG_RT_USING_DMA=y
CONFIG_RT_USING_DMA_PL330=y
CONFIG_RT_USING_DMA0=y
```

elm文件系统配置:

```shell
CONFIG_RT_USING_DFS=y
CONFIG_DFS_FILESYSTEMS_MAX=4
CONFIG_DFS_FILESYSTEM_TYPES_MAX=4
CONFIG_RT_USING_DFS_MNTTABLE=y
CONFIG_RT_USING_DFS_ELMFAT=y
CONFIG_RT_DFS_ELM_MAX_SECTOR_SIZE=4096
```

###### Flash分区表配置

File: SDK/device/rockchip/.chips/rk3308/parameters-rtos-amp.txt

```txt
... 0x00008000@0x00004000(amp),0x00800000@0x0000c000(rootfs),-@0x0080c000(userdata:grow)
```

Flash分区表中的rootfs字段，配置了Flash中文件系统分区的地址与大小，单位为512Byte。

###### 执行结果验证

固件升级首次启动后，需要先对文件系统格式化，操作如下：

```shell
mkfs -t elm sd0
```

重新启动后，文件系统挂载成功。通过文件系统串口命令操作，验证文件系统功能：

```shell
## 第8章 在根目录下创建一个文件
echo "This is a test!" /test.txt

## 第8章 查看目录
ls
Directory /:
test.txt

## 第8章 查看文件内容
cat test.txt
This is a test!
```

### SNOR

#### RK3308

##### RT-Thread

RT-Thread默认配置关闭SNOR Flash。如果用户需要在RT-Thread下使用SNOR Flash，按照以下方式修改：

Path: SDK/rtos/bsp/rockchip/rk3308-32

配置菜单配置：scons --menuconfig

###### RT-Thread配置

SNOR Flash配置:

```shell
CONFIG_RT_USING_MTD_NOR=y
CONFIG_RT_USING_SNOR=y
CONFIG_RT_USING_SNOR_SFC_HOST=y

```

elm文件系统配置:

```shell
CONFIG_RT_USING_DFS=y
CONFIG_DFS_FILESYSTEMS_MAX=4
CONFIG_DFS_FILESYSTEM_TYPES_MAX=4
CONFIG_RT_USING_DFS_MNTTABLE=y
CONFIG_RT_USING_DFS_ELMFAT=y
CONFIG_RT_DFS_ELM_MAX_SECTOR_SIZE=4096
```

###### Flash分区表配置

File: SDK/device/rockchip/.chips/rk3308/parameters-rtos-amp.txt

```txt
... 0x00002000@0x00004000(amp),0x00000800@0x00006000(rootfs),-@0x00006800(userdata:grow)
```

Flash分区表中的rootfs字段，配置了Flash中文件系统分区的地址与大小，单位为512Byte。

###### 执行结果验证

系统启动后，文件系统挂载成功。通过文件系统串口命令操作，验证文件系统功能：

```shell
## 第8章 查看目录
ls
Directory /:
demo.txt

## 第8章 查看文件内容
cat demo.txt
This is a demo for test.!
```

### GMAC

#### RK3308

##### RT-Thread

RT-Thread默认配置关闭GMAC。如果用户需要在RT-Thread下使用GMAC，按照以下方式修改：

Path: SDK/rtos/bsp/rockchip/rk3308-32

配置菜单配置：scons --menuconfig

###### RT-Thread配置

GMAC配置:

```shell
## 第8章 打开GMAC配置
CONFIG_RT_USING_GMAC=y
CONFIG_RT_USING_GMAC0=y

## 第8章 配置网关等信息
CONFIG_RT_LWIP_IPADDR="xx.xx.xx.xx"
CONFIG_RT_LWIP_GWADDR="xx.xx.xx.xx"
CONFIG_RT_LWIP_MSKADDR="255.255.255.0"

## 第8章 打开PING测试
CONFIG_RT_USING_NETUTILS=y
CONFIG_RT_NETUTILS_PING=y
```

###### 执行结果验证

验证前首先确认网线已经连接，再通过“ping”命令进行操作，参考如下：

```shell
## 第8章 网络连接成功log信息
[(1)3.357.573] e0: 100M
[(1)3.357.592] e0: full dumplex
[(1)3.357.610] e0: flow control off
[(1)3.357.811] e0: link up.

## 第8章 向网关发送ping包和执行结果
msh >ping 192.168.31.1
[(1)52.351.270] 60 bytes from 192.168.31.1 icmp_seq=0 ttl=64 time=0 ms
[(1)53.355.786] 60 bytes from 192.168.31.1 icmp_seq=1 ttl=64 time=0 ms
[(1)54.361.215] 60 bytes from 192.168.31.1 icmp_seq=2 ttl=64 time=0 ms
[(1)55.366.645] 60 bytes from 192.168.31.1 icmp_seq=3 ttl=64 time=0 ms
```

### PCIE

---


## 第10章 调试

### 串口调试

RK356X  AMP SDK中主核默认用UART2M0打印，从核默认用UART4M1打印 。

串口通信配置信息如下：

波特率：1500000

数据位：8

停止位：1

奇偶校验：none

流控：none

AMP系统启动后，以CPU1为例，能够看到如下u-boot打印：

```shell
AMP: Brought up cpu[100] with state 0x10, entry 0x01800000 ...OK
```

能够看到如下HAL打印：

```shell
****************************************
  Hello RK3568 Bare-metal using RK_HAL!
       Rockchip Electronics Co.Ltd
              CPI_ID(1)
****************************************
[(1) 0.002.772] CPU(1) Initial OK!
```

如果CPU运行的是RTT系统则能够看到如下RTT打印：

```
 \ | /
- RT -     Thread Operating System
 / | \     3.1.3 build Aug  24 2022
 2006 - 2019 Copyright by rt-thread team
 Hello RK3568 RT-Thread! CPU_ID(1)
```

### OpenOCD调试

RK356X AMP SDK 可以支持 JTAG 调试。当遇到复杂问题并且串口调试无法满足需求时，可以使用 JTAG 进行单步运行、断点追踪、data watch 和 寄存器、memory dump 等。

RK356X AMP SDK 的 JTAG 调试相关资料包可通过以下链接获取。

```shell
https://redmine.rock-chips.com/documents/111
```

#### 环境搭建说明

这里简单介绍 Windows 平台下的环境搭建说明。具体操作步骤及说明文档，请参考下载资料包下相关文档及说明。

- 安装 OpenOCD 开发环境

  OpenOCD 开发包为 openocd_eclipse-2020-09.zip，用户获取到资料包后，将该压缩文件解压至指定目录。

- 安装运行 eclipse 需要的 JRE 工具包

  JRE 工具包为下载资料包目录下的 /环境搭建软件/jdk_8.0.1310.11_64.exe。具体安装及配置步骤参见资料包下的《Rockchip_Developer_Guide_GNU_MCU_Eclipse_OpenOCD_CN.pdf》文档说明。

- 安装 JTAG 驱动

  JTAG 驱动为下载资料包目录下的 /环境搭建软件/zadig-2.7.exe。 具体安装及配置步骤参见资料包下的《Rockchip_Developer_Guide_FT232H_USB2JTAG.pdf》文档说明。

- RK3568 开发环境配置

  运行 openocd_eclipse-2020-09.zip 解压出的 eclipse.exe 文件，并参考《Rockchip_Developer_Guide_GNU_MCU_Eclipse_OpenOCD_CN.pdf》文档进行配置。

  这里对于部分配置简单说明，详细信息请参考以上文档：

    - 首先参照《Rockchip_Developer_Guide_GNU_MCU_Eclipse_OpenOCD_CN.pdf》文档，创建目标芯片的配置项。

    - 进入 “Debug Configurations” 配置项，打开 “Debugger” 标签页，在 “Config options” 项目中加入以下内容：

    ```shell
    -r rk3568
    -c "cpu0 aarch64_32"
    ```

    - 进入 “Debug Configurations” 配置项，打开 Source 标签页，编辑 “Path Mapping: New Mapping” 项目，加入或修改 RK356x AMP SDK 的工程路径，参考如下：

    ```shell
    /home/user/rk3568/hal/  # GCC 编译时的工程路径
    D:\rk3568\hal           # 用于 Debug 追踪的源代码的工程路径
    ```

    以上两个路径实际为同一路径，/home/user/rk3568/hal/ 为解析符号表时需要的路径信息。 D:\rk3568\hal\ 为 Windows 下加载工程源代码的路径。同时需要保证下载到开发板的固件，与调试代码一致。

    - 添加 elf 文件。以上配置完成后，通过 “Debug Configurations” 下的 “Debug” 按钮开始进行调试。此时会在 “Debugger Console” 窗口显示调试信息，如下：

    ```shell
    GNU gdb (GDB) 11.1
    Copyright (C) 2021 Free Software Foundation, Inc.
    License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
    This is free software: you are free to change and redistribute it.
    There is NO WARRANTY, to the extent permitted by law.
    Type "show copying" and "show warranty" for details.
    This GDB was configured as "--host=x86_64-w64-mingw32 --target=aarch64-none-elf".
    Type "show configuration" for configuration details.
    For bug reporting instructions, please see:
    <https://www.gnu.org/software/gdb/bugs/>.
    Find the GDB manual and other documentation resources online at:
        <http://www.gnu.org/software/gdb/documentation/>.

    For help, type "help".
    Type "apropos word" to search for commands related to "word".

    #......
    ```

    在该窗口下通过以下命令分别加入 4 个 CPU 的 *.elf 文件，参考如下：

    ```shell
    # ......
    For help, type "help".
    Type "apropos word" to search for commands related to "word".
    # ......
    add-symbol-file D:/rk3568/hal/project/rk3568/GCC/0_TestDemo.elf
    add-symbol-file D:/rk3568/hal/project/rk3568/GCC/1_TestDemo.elf
    add-symbol-file D:/rk3568/hal/project/rk3568/GCC/2_TestDemo.elf
    add-symbol-file D:/rk3568/hal/project/rk3568/GCC/3_TestDemo.elf
    ```

### Ozone调试

Ozone工具是一款常用的，功能强大，且带有便捷图像界面的嵌入式调试工具，借助J-Link硬件，可以实现对代码的实时跟踪，分步运行以及多断点触发等实用功能。官方地址：[Ozone – The Performance Analyzer (segger.com)](https://www.segger.com/products/development-tools/ozone-j-link-debugger/)。**官方支持两种使用许可模式（商业使用许可和非商业使用许可），请按实际需求，选择合适的许可模式合法使用。**

1. 连接好J-Link设备和调试板子，打开Ozone软件，默认跳出工程配置选项，或者点击`File->New->New Project Wizard`

2. 配置目标设备：在项目设置中配置目标设备和调试器选项。

3. 加载目标文件：使用Ozone的加载功能将目标文件（通常是生成的可执行文件）加载到调试器中。如果HAL代码仓库在Linux环境下，Ozone调试工具安装在Windows环境下，需要先在Windows系统中对Linux路径做网络磁盘映射，例如将Linux系统中的"/home/xxx"映射到Windows系统中的"Z:"。再在Ozone软件界面左下角命令行使用以下命令进行工程路径映射，其中参数"/home/xxx"为Linux环境挂载到Windows上使用的Linux路径，参数"Z:"则为对应的Windows路径。

   ```c
   Project.AddPathSubstitute "/home/xxx" "Z:"
   ```

4. 配置调试会话：在调试会话设置中选择断点、观察窗口等调试选项。

5. 启动调试会话：点击Ozone的调试按钮启动调试会话。

6. 调试应用程序：使用Ozone的调试功能来单步执行代码、查看变量值等。

7. 分析问题：如果遇到问题，可以使用Ozone的调试工具和功能来分析问题并找到解决方案。

Ozone调试器功能非常强大，除了常见的代码追踪功能外，还能查看CPU寄存器、直接读写Memory、通过Call Stack查看函数调用关系、跟踪变量等操作。详细使用说明可以参考Ozone官方手册[Getting started with Ozone (segger.com)](https://www.segger.com/products/development-tools/ozone-j-link-debugger/technology/getting-started-with-ozone/)。


---


## 第11章 演示

（需要提供演示固件）

### RK3568


### RK3308


### RK3358


### RK3562


---


## 第12章 附录

### 术语

### 文档索引


