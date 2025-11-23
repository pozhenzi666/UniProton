# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

UniProton 是一款实时操作系统(RTOS),具备极致的低时延和灵活的混合关键性部署特性,适用于工业控制场景,支持MCU和多核CPU。这是一个单进程多线程的操作系统。

## 支持的架构

- **ARMv7-M** (Cortex-M4)
- **ARMv7-R**
- **ARMv8** (AArch64)
- **RISCV64**
- **x86_64**

## 构建系统

### 推荐方式:使用Docker镜像

```bash
# 拉取镜像
docker pull swr.cn-north-4.myhuaweicloud.com/openeuler-embedded/uniproton:v004

# 创建并进入容器
docker run -it -v $(pwd):/home/uniproton swr.cn-north-4.myhuaweicloud.com/openeuler-embedded/uniproton:v004
```

### 构建命令

构建系统基于CMake。有两种构建方式:

1. **构建Demo应用** (推荐用于开发):
```bash
cd demos/<platform>/build
sh build_app.sh [app_name]
```

示例平台:
- `demos/m4/build` - Cortex-M4
- `demos/hi3093/build` - Hi3093 (ARMv8)
- `demos/raspi4/build` - Raspberry Pi 4
- `demos/x86_64/build` - x86_64

构建产物位置:
- 静态库: `demos/<platform>/libs/`
- 二进制文件: `demos/<platform>/build/`

2. **使用build.py构建OS内核**:
```bash
python build.py <cpu_type> [options]
```

### 测试

```bash
cd testsuites/build
sh build_app.sh        # ARM测试
sh build_app_x86_64.sh # x86_64测试
sh build_app_riscv64.sh # RISCV64测试
```

### 依赖项

编译器必须安装在 `/opt/buildtools/` 目录:
- Cortex-M4: `gcc-arm-none-eabi-10-2020-q4-major`
- ARMv8: `gcc-arm-10.3-2021.07-x86_64-aarch64-none-elf`
- RISCV64: `riscv64-elf-ubuntu-22.04-gcc-nightly-2023.12.20-nightly`
- x86_64: Docker容器中提供
- CMake 3.20.5+
- Python 3.4+

## 代码架构

### 核心子系统

UniProton由五大子系统构成:

1. **Mem** (`src/mem/`): 内存分区管理,内存块申请和释放
   - FSC算法实现在 `src/mem/fsc/`

2. **Arch** (`src/arch/`): 硬件架构相关功能
   - `src/arch/cpu/` 包含各架构适配代码
   - 硬中断、异常处理等

3. **Kernel** (`src/core/kernel/`): 核心内核功能
   - `task/` - 任务管理
   - `irq/` - 中断处理
   - `sched/` - 调度器
   - `timer/` - 软件定时器
   - `tick/` - TICK中断

4. **IPC** (`src/core/ipc/`): 进程间通信
   - 事件、队列、信号量

5. **OM** (`src/om/`): 运维和调试
   - `cpup/` - CPU占用率统计
   - `hook/` - 钩子函数
   - `err/` - 错误处理

### 其他重要目录

- `src/libc/` - POSIX和C库支持
  - `musl/` - musl 1.2.3源码
  - `litelibc/` - musl到UniProton的适配层
- `src/kal/` - POSIX适配中间层
- `src/fs/` - 文件系统(littlefs, vfs)
- `src/net/` - 网络栈(lwip适配)
- `src/drivers/` - 驱动框架
- `src/osal/` - 操作系统抽象层(C++, Linux接口支持)
- `src/security/` - 安全功能(随机数等)
- `src/shell/` - Shell支持
- `platform/libboundscheck/` - 边界检查库

### 配置和构建

- `config.xml` - 主配置文件,定义各平台的编译器路径和配置
- `build/uniproton_config/config_<arch>/` - 各架构的功能宏配置
- `cmake/` - CMake构建脚本和工具链配置
- `demos/` - 示例应用,每个demo包含:
  - `apps/` - 应用代码和main函数
  - `bsp/` - 板级驱动适配
  - `build/` - 构建脚本和链接脚本
  - `config/` - 用户配置和功能宏
  - `libs/` - 编译输出的静态库

## 任务调度机制

- **抢占式调度**: 高优先级任务可打断低优先级任务
- **优先级**: 数字越小优先级越高
- **IDLE任务**: 系统默认创建最低优先级的后台任务
- 不使用时间片轮转调度

## Git提交规范

遵循openEuler的git commit规范:
https://openeuler.gitee.io/yocto-meta-openeuler/master/develop_help/commit.html

## 相关文档

- 用户指南: https://docs.openeuler.org/zh/docs/23.09/docs/Embedded/UniProton/
- 接口说明: https://docs.openeuler.org/zh/docs/23.09/docs/Embedded/UniProton/UniProton接口说明.html
- 混合部署框架: https://gitee.com/openeuler/mcs
- 特性文档: `doc/design/` 目录包含各功能模块设计文档
