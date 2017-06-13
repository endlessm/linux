.. -*- coding: utf-8; mode: rst -*-

.. _V4L2_PIX_FMT_IPU3_SBGGR10:
.. _V4L2_PIX_FMT_IPU3_SGBRG10:
.. _V4L2_PIX_FMT_IPU3_SGRBG10:
.. _V4L2_PIX_FMT_IPU3_SRGGB10:

**********************************************************************************************************************************************
V4L2_PIX_FMT_IPU3_SBGGR10 ('ip3b'), V4L2_PIX_FMT_IPU3_SGBRG10 ('ip3g'), V4L2_PIX_FMT_IPU3_SGRBG10 ('ip3G'), V4L2_PIX_FMT_IPU3_SRGGB10 ('ip3r')
**********************************************************************************************************************************************

10-bit Bayer formats

Description
===========

These four pixel formats are used by Intel IPU3 driver, they are raw
sRGB / Bayer formats with 10 bits per sample with every 25 pixels packed
to 32 bytes leaving 6 most significant bits padding in the last byte.
The format is little endian.

In other respects this format is similar to :ref:`V4L2-PIX-FMT-SRGGB10`.

**Byte Order.**
Each cell is one byte.

.. raw:: latex

    \newline\newline\begin{adjustbox}{width=\columnwidth}

.. tabularcolumns:: |p{1.3cm}|p{1.0cm}|p{10.9cm}|p{10.9cm}|p{10.9cm}|p{1.0cm}|

.. flat-table::

    * - start + 0:
      - B\ :sub:`00low`
      - G\ :sub:`01low` \ (bits 7--2) B\ :sub:`00high`\ (bits 1--0)
      - B\ :sub:`02low` \ (bits 7--4) G\ :sub:`01high`\ (bits 3--0)
      - G\ :sub:`03low` \ (bits 7--6) B\ :sub:`02high`\ (bits 5--0)
      - G\ :sub:`03high`
    * - start + 5:
      - G\ :sub:`10low`
      - R\ :sub:`11low` \ (bits 7--2) G\ :sub:`10high`\ (bits 1--0)
      - G\ :sub:`12low` \ (bits 7--4) R\ :sub:`11high`\ (bits 3--0)
      - R\ :sub:`13low` \ (bits 7--6) G\ :sub:`12high`\ (bits 5--0)
      - R\ :sub:`13high`
    * - start + 10:
      - B\ :sub:`20low`
      - G\ :sub:`21low` \ (bits 7--2) B\ :sub:`20high`\ (bits 1--0)
      - B\ :sub:`22low` \ (bits 7--4) G\ :sub:`21high`\ (bits 3--0)
      - G\ :sub:`23low` \ (bits 7--6) B\ :sub:`22high`\ (bits 5--0)
      - G\ :sub:`23high`
    * - start + 15:
      - G\ :sub:`30low`
      - R\ :sub:`31low` \ (bits 7--2) G\ :sub:`30high`\ (bits 1--0)
      - G\ :sub:`32low` \ (bits 7--4) R\ :sub:`31high`\ (bits 3--0)
      - R\ :sub:`33low` \ (bits 7--6) G\ :sub:`32high`\ (bits 5--0)
      - R\ :sub:`33high`

.. raw:: latex

    \end{adjustbox}\newline\newline
