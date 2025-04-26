# RTMP2HLS

[English](#english) | [中文](#chinese)

<a name="english"></a>

## Overview
RTMP2HLS is a high-performance streaming media conversion tool that converts RTMP streams to HLS format. It supports parallel processing of multiple tasks and provides simple task management through CSV configuration.

### Features
- RTMP to HLS stream conversion
- Multi-task parallel processing
- Simple task management based on CSV configuration
- Cross-platform support (Linux systems)
- Real-time stream monitoring and processing

### Requirements
- GCC/G++ compiler
- FFmpeg (for media stream conversion)
- Linux operating system

### Compilation
```bash
# 1. Configure the project
./configure

# 2. Build the project
make
```
After compilation, the executable file `rtmp2hls` will be generated in the project root directory.

### Project Structure
```
rtmp2hls/
├── bin/           # Binary files directory
├── html/          # HLS output directory
├── logs/          # Log directory
└── src/           # Source code directory
    ├── common/    # Common utilities
    ├── core/      # Core functionality
    ├── http/      # HTTP server components
    ├── process/   # Process management
    └── utils/     # Utility functions
```

### Third-party Libraries
- FFmpeg: Media processing framework
- cpp-httplib: HTTP server implementation
- CSV Parser: Task configuration file parsing

---

<a name="chinese"></a>

## 概述
RTMP2HLS是一个高性能的流媒体转换工具，用于将RTMP流转换为HLS格式。它支持多任务并行处理，并通过CSV配置提供简单的任务管理。

### 功能特点
- RTMP流转HLS流转换
- 多任务并行处理
- 基于CSV配置的简单任务管理
- 跨平台支持（Linux系统）
- 实时流监控和处理

### 环境要求
- GCC/G++编译器
- FFmpeg（用于流媒体转换）
- Linux操作系统

### 编译说明
```bash
# 1. 配置项目
./configure

# 2. 编译项目
make
```
编译完成后，可执行文件`rtmp2hls`将生成在项目根目录。

### 项目结构
```
rtmp2hls/
├── bin/           # 二进制文件目录
├── html/          # HLS输出目录
├── logs/          # 日志目录
└── src/           # 源代码目录
    ├── common/    # 通用工具
    ├── core/      # 核心功能
    ├── http/      # HTTP服务器组件
    ├── process/   # 进程管理
    └── utils/     # 工具函数
```

### 第三方库
- FFmpeg：媒体处理框架
- cpp-httplib：HTTP服务器实现
- CSV Parser：任务配置文件解析
