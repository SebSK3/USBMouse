# USBMouse Linux Kernel Module

## Overview

USBMouse is a simple Linux kernel module that provides basic support for USB mice with USB ID 3151:3020 in the Linux USB subsystem using USB Request Blocks (URBs). The driver supports X-Y movements, button clicks and the scroll wheel.

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)


## Installation

1. Clone the USBMouse repository to your local machine:

    ```shell
    git clone https://github.com/garyli2/USBMouse.git
    ```

2. Build the kernel module:

    ```shell
    make
    ```

3. Load the USBMouse module:

    ```shell
    insmod mouse.ko
    ```

## Usage

The driver will probe connected USB devices and accept ones with ID 3151:3020. You may need to unbind these devices from the generic linux drivers first.