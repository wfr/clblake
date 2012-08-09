=========
 clblake
=========

------------------------------------------------------------
 Experimental tree hasher using an OpenCL port of BLAKE-256
------------------------------------------------------------

:date: $Date: Thu, 09 Aug 2012 16:10:51 +0200 $


About
=====

*This program a rough draft, a prototype. It will eat your hamster.*

It includes 

* a simplified OpenCL (OpenCL_) implementation of the BLAKE-256 (BLAKEWikipedia_) algorithm

* C, SSE2 and SSSE3 optimized CPU implementations of BLAKE-256

The "tree" can be tuned statically in BlakeTree.h.  Its height is only 2 at the moment.
Do note that the tree configuration affects the resulting hash.

This program is inspired by Keccak.Tree.GPU (KeccakTreeGPU_).

Tree hashing is implemented both for OpenCL and the CPU. The latter uses OpenMP (OpenMP_).


.. _OpenCL: http://www.khronos.org/opencl/

.. _BLAKEWikipedia: http://en.wikipedia.org/wiki/BLAKE_%28hash_function%29

.. _SuperCop: http://bench.cr.yp.to/supercop.html

.. _KeccakTreeGPU: https://sites.google.com/site/keccaktreegpu/

.. _OpenMP: http://openmp.org/wp/


Requirements
============

* A GPU with OpenCL support

  only tested with the nvidia blob on GNU/Linux, amd64 architecture.

* GCC 4 and GNU Make

* OpenCL development libs/headers

* glib (only used for the FIFO queue, can be replaced easily)


Examples
========

GPU test
--------
``./blaketree -v -t``

Output:

::
    
    DEBUG: Blake-256 CPU implementation: SSSE3
    CPU hash test...
    CPU hash is valid
    GPU hash test...
    DEBUG: Using platform: NVIDIA CUDA
    DEBUG: Using OpenCL source: blake256.cl
    DEBUG: 1716.2 MiB/s


Using sparse files to test the optimal file throughput
------------------------------------------------------
::
    
    truncate -s 1G zero.1GiB

Hashing a file on the GPU
-------------------------

``./blaketree -v zero.1GiB``

Output:

::

    DEBUG: Blake-256 CPU implementation: SSSE3
    DEBUG: Using platform: NVIDIA CUDA
    DEBUG: Using OpenCL source: blake256.cl
    da282d6960ede0b5fc7972916b72f1fef4fbf56898ee014a0af7adf0db3af50d
    DEBUG: 975.5 MiB/s

Hashing a file on the CPU
-------------------------

``./blaketree -v -c zero.1GiB``

Output: 

::

    DEBUG: Blake-256 CPU implementation: SSSE3
    da282d6960ede0b5fc7972916b72f1fef4fbf56898ee014a0af7adf0db3af50d
    DEBUG: 435.9 MiB/s



License
=======

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see (http://www.gnu.org/licenses/).

3rd party code
--------------

The program includes modified 3rd party code:

* BLAKE-256 SSE2 and SSSE3 optimizations from (SuperCop_).

  License: public domain

* The BLAKE-256 reference implementation.

  License: public domain
