.. SPDX-License-Identifier: GPL-2.0

.. include:: <isonum.txt>

====================================
AMD Image Signal Processor (amdisp4)
====================================

Introduction
============

This file documents the driver for the AMD ISP4 that is part of
AMD Ryzen AI Max 385 SoC.

The driver is located under drivers/media/platform/amd/isp4 and uses
the Media-Controller API.

Topology
========

.. _amdisp4_topology_graph:

.. kernel-figure:: amdisp4.dot
     :alt:   Diagram of the media pipeline topology
     :align: center



The driver has 1 sub-device:

- isp: used to resize and process bayer raw frames in to yuv.

The driver has 1 video device:

- capture video device: capture device for retrieving images.


  - ISP4 Image Signal Processing Subdevice Node

-----------------------------------------------

The isp4 is represented as a single V4L2 subdev, the sub-device does not
provide interface to the user space. The sub-device is connected to one video node
(isp4_capture) with immutable active link. The isp entity is connected
to sensor pad 0 and receives the frames using CSI-2 protocol. The sub-device is
also responsible to configure CSI2-2 receiver.
The sub-device processes bayer raw data from the connected sensor and output
them to different YUV formats. The isp also has scaling capabilities.

  - isp4_capture - Frames Capture Video Node

--------------------------------------------

Isp4_capture is a capture device to capture frames to memory.
This entity is the DMA engine that write the frames to memory.
The entity is connected to isp4 sub-device.

Capturing Video Frames Example
==============================

.. code-block:: bash

         # set the links

         # start streaming:
         v4l2-ctl "-d" "/dev/video0" "--set-fmt-video=width=1920,height=1080,pixelformat=NV12" "--stream-mmap" "--stream-count=10"
