DPDK Release 16.11
==================

.. **Read this first.**

   The text below explains how to update the release notes.

   Use proper spelling, capitalization and punctuation in all sections.

   Variable and config names should be quoted as fixed width text: ``LIKE_THIS``.

   Build the docs and view the output file to ensure the changes are correct::

      make doc-guides-html

      firefox build/doc/html/guides/rel_notes/release_16_11.html


New Features
------------

.. This section should contain new features added in this release. Sample format:

   * **Add a title in the past tense with a full stop.**

     Add a short 1-2 sentence description in the past tense. The description
     should be enough to allow someone scanning the release notes to understand
     the new feature.

     If the feature adds a lot of sub-features you can use a bullet list like this.

     * Added feature foo to do something.
     * Enhanced feature bar to do something else.

     Refer to the previous release notes for examples.

     This section is a comment. Make sure to start the actual text at the margin.


* **Added software parser for packet type.**

  * Added a new function ``rte_pktmbuf_read()`` to read the packet data from an
    mbuf chain, linearizing if required.
  * Added a new function ``rte_net_get_ptype()`` to parse an Ethernet packet
    in an mbuf chain and retrieve its packet type from software.
  * Added new functions ``rte_get_ptype_*()`` to dump a packet type as a string.

* **Improved offloads support in mbuf.**

  * Added a new function ``rte_raw_cksum_mbuf()`` to process the checksum of
    data embedded in an mbuf chain.
  * Added new Rx checksum flags in mbufs to describe more states: unknown,
    good, bad, or not present (useful for virtual drivers). This modification
    was done for IP and L4.
  * Added a new Rx LRO mbuf flag, used when packets are coalesced. This
    flag indicates that the segment size of original packets is known.

* **Added vhost-user dequeue zero copy support.**

  The copy in the dequeue path is avoided in order to improve the performance.
  In the VM2VM case, the boost is quite impressive. The bigger the packet size,
  the bigger performance boost you may get. However, for the VM2NIC case, there
  are some limitations, so the boost is not as  impressive as the VM2VM case.
  It may even drop quite a bit for small packets.

  For that reason, this feature is disabled by default. It can be enabled when
  the ``RTE_VHOST_USER_DEQUEUE_ZERO_COPY`` flag is set. Check the VHost section
  of the Programming Guide for more information.

* **Added vhost-user indirect descriptors support.**

  If the indirect descriptor feature is enabled, each packet sent by the guest
  will take exactly one slot in the enqueue virtqueue. Without this feature, as in
  the current version, even 64 bytes packets take two slots with Virtio PMD on guest
  side.

  The main impact is better performance for 0% packet loss use-cases, as it
  behaves as if the virtqueue size was enlarged, so more packets can be buffered
  in the case of system perturbations. On the downside, small performance degradations
  were measured when running micro-benchmarks.

* **Added vhost PMD xstats.**

  Added extended statistics to vhost PMD from a per port perspective.

* **Supported offloads with virtio.**

  Added support for the following offloads in virtio:

  * Rx/Tx checksums.
  * LRO.
  * TSO.

* **Added virtio NEON support for ARM.**

  Added NEON support for ARM based virtio.

* **Updated the ixgbe base driver.**

  Updated the ixgbe base driver, including the following changes:

  * Added X550em_a 10G PHY support.
  * Added support for flow control auto negotiation for X550em_a 1G PHY.
  * Added X550em_a FW ALEF support.
  * Increased mailbox version to ``ixgbe_mbox_api_13``.
  * Added two MAC operations for Hyper-V support.

* **Added APIs for VF management to the ixgbe PMD.**

  Eight new APIs have been added to the ixgbe PMD for VF management from the PF.
  The declarations for the API's can be found in ``rte_pmd_ixgbe.h``.

* **Updated the enic driver.**

  * Added update to use interrupt for link status checking instead of polling.
  * Added more flow director modes on UCS Blade with firmware version >= 2.0(13e).
  * Added full support for MTU update.
  * Added support for the ``rte_eth_rx_queue_count`` function.

* **Updated the mlx5 driver.**

  * Added support for RSS hash results.
  * Added several performance improvements.
  * Added several bug fixes.

* **Updated the QAT PMD.**

  The QAT PMD was updated with additional support for:

  * MD5_HMAC algorithm.
  * SHA224-HMAC algorithm.
  * SHA384-HMAC algorithm.
  * GMAC algorithm.
  * KASUMI (F8 and F9) algorithm.
  * 3DES algorithm.
  * NULL algorithm.
  * C3XXX device.
  * C62XX device.

* **Added openssl PMD.**

  A new crypto PMD has been added, which provides several ciphering and hashing algorithms.
  All cryptography operations use the Openssl library crypto API.

* **Updated the IPsec example.**

  Updated the IPsec example with the following support:

  * Configuration file support.
  * AES CBC IV generation with cipher forward function.
  * AES GCM/CTR mode.

* **Added support for new gcc -march option.**

  The GCC 4.9 ``-march`` option supports the Intel processor code names.
  The config option ``RTE_MACHINE`` can be used to pass code names to the compiler via the ``-march`` flag.


Resolved Issues
---------------

.. This section should contain bug fixes added to the relevant sections. Sample format:

   * **code/section Fixed issue in the past tense with a full stop.**

     Add a short 1-2 sentence description of the resolved issue in the past tense.
     The title should contain the code/lib section like a commit message.
     Add the entries in alphabetic order in the relevant sections below.

   This section is a comment. Make sure to start the actual text at the margin.


Drivers
~~~~~~~

* **enic: Fixed several flow director issues.**

* **enic: Fixed inadvertent setting of L4 checksum ptype on ICMP packets.**

* **enic: Fixed high driver overhead when servicing Rx queues beyond the first.**



Known Issues
------------

.. This section should contain new known issues in this release. Sample format:

   * **Add title in present tense with full stop.**

     Add a short 1-2 sentence description of the known issue in the present
     tense. Add information on any known workarounds.

   This section is a comment. Make sure to start the actual text at the margin.

* **L3fwd-power app does not work properly when Rx vector is enabled.**

  The L3fwd-power app doesn't work properly with some drivers in vector mode
  since the queue monitoring works differently between scalar and vector modes
  leading to incorrect frequency scaling. In addition, L3fwd-power application
  requires the mbuf to have correct packet type set but in some drivers the
  vector mode must be disabled for this.

  Therefore, in order to use L3fwd-power, vector mode should be disabled
  via the config file.

* **Digest address must be supplied for crypto auth operation on QAT PMD.**

  The cryptodev API specifies that if the rte_crypto_sym_op.digest.data field,
  and by inference the digest.phys_addr field which points to the same location,
  is not set for an auth operation the driver is to understand that the digest
  result is located immediately following the region over which the digest is
  computed. The QAT PMD doesn't correctly handle this case and reads and writes
  to an incorrect location.

  Callers can workaround this by always supplying the digest virtual and
  physical address fields in the rte_crypto_sym_op for an auth operation.


API Changes
-----------

.. This section should contain API changes. Sample format:

   * Add a short 1-2 sentence description of the API change. Use fixed width
     quotes for ``rte_function_names`` or ``rte_struct_names``. Use the past tense.

   This section is a comment. Make sure to start the actual text at the margin.

* The driver naming convention has been changed to make them more
  consistent. It especially impacts ``--vdev`` arguments. For example
  ``eth_pcap`` becomes ``net_pcap`` and ``cryptodev_aesni_mb_pmd`` becomes
  ``crypto_aesni_mb``.

  For backward compatibility an alias feature has been enabled to support the
  original names.

* The log history has been removed.

* The ``rte_ivshmem`` feature (including library and EAL code) has been removed
  in 16.11 because it had some design issues which were not planned to be fixed.

* The ``file_name`` data type of ``struct rte_port_source_params`` and
  ``struct rte_port_sink_params`` is changed from ``char *`` to ``const char *``.

* **Improved device/driver hierarchy and generalized hotplugging.**

  The device and driver relationship has been restructured by introducing generic
  classes. This paves the way for having PCI, VDEV and other device types as
  instantiated objects rather than classes in themselves. Hotplugging has also
  been generalized into EAL so that Ethernet or crypto devices can use the
  common infrastructure.

  * Removed ``pmd_type`` as a way of segregation of devices.
  * Moved ``numa_node`` and ``devargs`` into ``rte_driver`` from
    ``rte_pci_driver``. These can now be used by any instantiated object of
    ``rte_driver``.
  * Added ``rte_device`` class and all PCI and VDEV devices inherit from it
  * Renamed devinit/devuninit handlers to probe/remove to make it more
    semantically correct with respect to the device <=> driver relationship.
  * Moved hotplugging support to EAL. Hereafter, PCI and vdev can use the
    APIs ``rte_eal_dev_attach`` and ``rte_eal_dev_detach``.
  * Renamed helpers and support macros to make them more synonymous
    with their device types
    (e.g. ``PMD_REGISTER_DRIVER`` => ``RTE_PMD_REGISTER_PCI``).
  * Device naming functions have been generalized from ethdev and cryptodev
    to EAL. ``rte_eal_pci_device_name`` has been introduced for obtaining
    unique device name from PCI Domain-BDF description.
  * Virtual device registration APIs have been added: ``rte_eal_vdrv_register``
    and ``rte_eal_vdrv_unregister``.


ABI Changes
-----------

.. This section should contain ABI changes. Sample format:

   * Add a short 1-2 sentence description of the ABI change that was announced in
     the previous releases and made in this release. Use fixed width quotes for
     ``rte_function_names`` or ``rte_struct_names``. Use the past tense.

   This section is a comment. Make sure to start the actual text at the margin.



Shared Library Versions
-----------------------

.. Update any library version updated in this release and prepend with a ``+``
   sign, like this:

     libethdev.so.4
     librte_acl.so.2
   + librte_cfgfile.so.2
     librte_cmdline.so.2



The libraries prepended with a plus sign were incremented in this version.

.. code-block:: diff

     librte_acl.so.2
     librte_cfgfile.so.2
     librte_cmdline.so.2
   + librte_cryptodev.so.2
     librte_distributor.so.1
   + librte_eal.so.3
   + librte_ethdev.so.5
     librte_hash.so.2
     librte_ip_frag.so.1
     librte_jobstats.so.1
     librte_kni.so.2
     librte_kvargs.so.1
     librte_lpm.so.2
     librte_mbuf.so.2
     librte_mempool.so.2
     librte_meter.so.1
     librte_net.so.1
     librte_pdump.so.1
     librte_pipeline.so.3
     librte_pmd_bond.so.1
     librte_pmd_ring.so.2
     librte_port.so.3
     librte_power.so.1
     librte_reorder.so.1
     librte_ring.so.1
     librte_sched.so.1
     librte_table.so.2
     librte_timer.so.1
     librte_vhost.so.3


Tested Platforms
----------------

.. This section should contain a list of platforms that were tested with this release.

   The format is:

   #. Platform name.

      * Platform details.
      * Platform details.

   This section is a comment. Make sure to start the actual text at the margin.

#. SuperMicro 1U

   - BIOS: 1.0c
   - Processor: Intel(R) Atom(TM) CPU C2758 @ 2.40GHz

#. SuperMicro 1U

   - BIOS: 1.0a
   - Processor: Intel(R) Xeon(R) CPU D-1540 @ 2.00GHz
   - Onboard NIC: Intel(R) X552/X557-AT (2x10G)

     - Firmware-version: 0x800001cf
     - Device ID (PF/VF): 8086:15ad /8086:15a8

   - kernel driver version: 4.2.5 (ixgbe)

#. SuperMicro 2U

   - BIOS: 1.0a
   - Processor: Intel(R) Xeon(R) CPU E5-4667 v3 @ 2.00GHz

#. Intel(R) Server board S2600GZ

   - BIOS: SE5C600.86B.02.02.0002.122320131210
   - Processor: Intel(R) Xeon(R) CPU E5-2680 v2 @ 2.80GHz

#. Intel(R) Server board W2600CR

   - BIOS: SE5C600.86B.02.01.0002.082220131453
   - Processor: Intel(R) Xeon(R) CPU E5-2680 v2 @ 2.80GHz

#. Intel(R) Server board S2600CWT

   - BIOS: SE5C610.86B.01.01.0009.060120151350
   - Processor: Intel(R) Xeon(R) CPU E5-2699 v3 @ 2.30GHz

#. Intel(R) Server board S2600WTT

   - BIOS: SE5C610.86B.01.01.0005.101720141054
   - Processor: Intel(R) Xeon(R) CPU E5-2699 v3 @ 2.30GHz

#. Intel(R) Server board S2600WTT

   - BIOS: SE5C610.86B.11.01.0044.090120151156
   - Processor: Intel(R) Xeon(R) CPU E5-2695 v4 @ 2.10GHz

#. Intel(R) Server board S2600WTT

   - Processor: Intel(R) Xeon(R) CPU E5-2697 v2 @ 2.70GHz

#. Intel(R) Server

   - Intel(R) Xeon(R) CPU E5-2697 v3 @ 2.60GHz

#. IBM(R) Power8(R)

   - Machine type-model: 8247-22L
   - Firmware FW810.21 (SV810_108)
   - Processor: POWER8E (raw), AltiVec supported


Tested NICs
-----------

.. This section should contain a list of NICs that were tested with this release.

   The format is:

   #. NIC name.

      * NIC details.
      * NIC details.

   This section is a comment. Make sure to start the actual text at the margin.

#. Intel(R) Ethernet Controller X540-AT2

   - Firmware version: 0x80000389
   - Device id (pf): 8086:1528
   - Driver version: 3.23.2 (ixgbe)

#. Intel(R) 82599ES 10 Gigabit Ethernet Controller

   - Firmware version: 0x61bf0001
   - Device id (pf/vf): 8086:10fb / 8086:10ed
   - Driver version: 4.0.1-k (ixgbe)

#. Intel(R) Corporation Ethernet Connection X552/X557-AT 10GBASE-T

   - Firmware version: 0x800001cf
   - Device id (pf/vf): 8086:15ad / 8086:15a8
   - Driver version: 4.2.5 (ixgbe)

#. Intel(R) Ethernet Converged Network Adapter X710-DA4 (4x10G)

   - Firmware version: 5.05
   - Device id (pf/vf): 8086:1572 / 8086:154c
   - Driver version: 1.5.23 (i40e)

#. Intel(R) Ethernet Converged Network Adapter X710-DA2 (2x10G)

   - Firmware version: 5.05
   - Device id (pf/vf): 8086:1572 / 8086:154c
   - Driver version: 1.5.23 (i40e)

#. Intel(R) Ethernet Converged Network Adapter XL710-QDA1 (1x40G)

   - Firmware version: 5.05
   - Device id (pf/vf): 8086:1584 / 8086:154c
   - Driver version: 1.5.23 (i40e)

#. Intel(R) Ethernet Converged Network Adapter XL710-QDA2 (2X40G)

   - Firmware version: 5.05
   - Device id (pf/vf): 8086:1583 / 8086:154c
   - Driver version: 1.5.23 (i40e)

#. Intel(R) Corporation I350 Gigabit Network Connection

   - Firmware version: 1.48, 0x800006e7
   - Device id (pf/vf): 8086:1521 / 8086:1520
   - Driver version: 5.2.13-k (igb)

#. Intel(R) Ethernet Multi-host Controller FM10000

   - Firmware version: N/A
   - Device id (pf/vf): 8086:15d0
   - Driver version: 0.17.0.9 (fm10k)

#. Mellanox(R) ConnectX(R)-4 10G MCX4111A-XCAT (1x10G)

   * Host interface: PCI Express 3.0 x8
   * Device ID: 15b3:1013
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 12.17.1010

#. Mellanox(R) ConnectX(R)-4 10G MCX4121A-XCAT (2x10G)

   * Host interface: PCI Express 3.0 x8
   * Device ID: 15b3:1013
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 12.17.1010

#. Mellanox(R) ConnectX(R)-4 25G MCX4111A-ACAT (1x25G)

   * Host interface: PCI Express 3.0 x8
   * Device ID: 15b3:1013
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 12.17.1010

#. Mellanox(R) ConnectX(R)-4 25G MCX4121A-ACAT (2x25G)

   * Host interface: PCI Express 3.0 x8
   * Device ID: 15b3:1013
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 12.17.1010

#. Mellanox(R) ConnectX(R)-4 40G MCX4131A-BCAT/MCX413A-BCAT (1x40G)

   * Host interface: PCI Express 3.0 x8
   * Device ID: 15b3:1013
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 12.17.1010

#. Mellanox(R) ConnectX(R)-4 40G MCX415A-BCAT (1x40G)

   * Host interface: PCI Express 3.0 x16
   * Device ID: 15b3:1013
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 12.17.1010

#. Mellanox(R) ConnectX(R)-4 50G MCX4131A-GCAT/MCX413A-GCAT (1x50G)

   * Host interface: PCI Express 3.0 x8
   * Device ID: 15b3:1013
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 12.17.1010

#. Mellanox(R) ConnectX(R)-4 50G MCX414A-BCAT (2x50G)

   * Host interface: PCI Express 3.0 x8
   * Device ID: 15b3:1013
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 12.17.1010

#. Mellanox(R) ConnectX(R)-4 50G MCX415A-GCAT/MCX416A-BCAT/MCX416A-GCAT (2x50G)

   * Host interface: PCI Express 3.0 x16
   * Device ID: 15b3:1013
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 12.17.1010

#. Mellanox(R) ConnectX(R)-4 50G MCX415A-CCAT (1x100G)

   * Host interface: PCI Express 3.0 x16
   * Device ID: 15b3:1013
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 12.17.1010

#. Mellanox(R) ConnectX(R)-4 100G MCX416A-CCAT (2x100G)

   * Host interface: PCI Express 3.0 x16
   * Device ID: 15b3:1013
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 12.17.1010

#. Mellanox(R) ConnectX(R)-4 Lx 10G MCX4121A-XCAT (2x10G)

   * Host interface: PCI Express 3.0 x8
   * Device ID: 15b3:1015
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 14.17.1010

#. Mellanox(R) ConnectX(R)-4 Lx 25G MCX4121A-ACAT (2x25G)

   * Host interface: PCI Express 3.0 x8
   * Device ID: 15b3:1015
   * MLNX_OFED: 3.4-1.0.0.0
   * Firmware version: 14.17.1010


Tested OSes
-----------

.. This section should contain a list of OSes that were tested with this release.
   The format is as follows, in alphabetical order:

   * CentOS 7.0
   * Fedora 23
   * Fedora 24
   * FreeBSD 10.3
   * Red Hat Enterprise Linux 7.2
   * SUSE Enterprise Linux 12
   * Ubuntu 15.10
   * Ubuntu 16.04 LTS
   * Wind River Linux 8

   This section is a comment. Make sure to start the actual text at the margin.

* CentOS 7.2
* Fedora 23
* Fedora 24
* FreeBSD 10.3
* FreeBSD 11
* Red Hat Enterprise Linux Server release 6.7 (Santiago)
* Red Hat Enterprise Linux Server release 7.0 (Maipo)
* Red Hat Enterprise Linux Server release 7.2 (Maipo)
* SUSE Enterprise Linux 12
* Wind River Linux 6.0.0.26
* Wind River Linux 8
* Ubuntu 14.04
* Ubuntu 15.04
* Ubuntu 16.04

Fixes in 16.11 LTS Release
--------------------------

16.11.1
~~~~~~~

* app/test: fix symmetric session free in crypto perf tests
* app/testpmd: fix check for invalid ports
* app/testpmd: fix static build link ordering
* crypto/aesni_gcm: fix IV size in capabilities
* crypto/aesni_gcm: fix J0 padding bytes
* crypto/aesni_mb: fix incorrect crypto session
* crypto/openssl: fix extra bytes written at end of data
* crypto/openssl: fix indentation in guide
* crypto/qat: fix IV size in capabilities
* crypto/qat: fix to avoid buffer overwrite in OOP case
* cryptodev: fix crash on null dereference
* cryptodev: fix loop in device query
* devargs: reset driver name pointer on parsing failure
* drivers/crypto: fix different auth/cipher keys
* ethdev: check maximum number of queues for statistics
* ethdev: fix extended statistics name index
* ethdev: fix port data mismatched in multiple process model
* ethdev: fix port lookup if none
* ethdev: remove invalid function from version map
* examples/ethtool: fix driver information
* examples/ethtool: fix querying non-PCI devices
* examples/ip_pipeline: fix coremask limitation
* examples/ip_pipeline: fix parsing of pass-through pipeline
* examples/l2fwd-crypto: fix overflow
* examples/vhost: fix calculation of mbuf count
* examples/vhost: fix lcore initialization
* mempool: fix API documentation
* mempool: fix stack handler dequeue
* net/af_packet: fix fd use after free
* net/bnx2x: fix Rx mode configuration
* net/cxgbe/base: initialize variable before reading EEPROM
* net/cxgbe: fix parenthesis on bitwise operation
* net/ena: fix setting host attributes
* net/enic: fix hardcoding of some flow director masks
* net/enic: fix memory leak with oversized Tx packets
* net/enic: remove unnecessary function parameter attributes
* net/i40e: enable auto link update for 25G
* net/i40e: fix Rx checksum flag
* net/i40e: fix TC bandwidth definition
* net/i40e: fix VF reset flow
* net/i40e: fix checksum flag in x86 vector Rx
* net/i40e: fix crash in close
* net/i40e: fix deletion of all macvlan filters
* net/i40e: fix ethertype filter on X722
* net/i40e: fix link update delay
* net/i40e: fix logging for Tx free threshold check
* net/i40e: fix segment number in reassemble process
* net/i40e: fix wrong return value when handling PF message
* net/i40e: fix xstats value mapping
* net/i40evf: fix casting between structs
* net/i40evf: fix reporting of imissed packets
* net/ixgbe: fix blocked interrupts
* net/ixgbe: fix received packets number for ARM
* net/ixgbe: fix received packets number for ARM NEON
* net/ixgbevf: fix max packet length
* net/mlx5: fix RSS hash result for flows
* net/mlx5: fix Rx packet validation and type
* net/mlx5: fix Tx doorbell
* net/mlx5: fix endianness in Tx completion queue
* net/mlx5: fix inconsistent link status
* net/mlx5: fix leak when starvation occurs
* net/mlx5: fix link status query
* net/mlx5: fix memory leak when parsing device params
* net/mlx5: fix missing inline attributes
* net/mlx5: fix updating total length of multi-packet send
* net/mlx: fix IPv4 and IPv6 packet type
* net/nfp: fix VLAN offload flags check
* net/nfp: fix typo in Tx offload capabilities
* net/pcap: fix timestamps in output pcap file
* net/qede/base: fix FreeBSD build
* net/qede: add vendor/device id info
* net/qede: fix PF fastpath status block index
* net/qede: fix filtering code
* net/qede: fix function declaration
* net/qede: fix per queue statisitics
* net/qede: fix resource leak
* net/vhost: fix socket file deleted on stop
* net/vhost: fix unix socket not removed as closing
* net/virtio-user: fix not properly reset device
* net/virtio-user: fix wrongly get/set features
* net/virtio: fix build without virtio-user
* net/virtio: fix crash when number of virtio devices > 1
* net/virtio: fix multiple process support
* net/virtio: fix performance regression due to TSO
* net/virtio: fix rewriting LSC flag
* net/virtio: fix wrong Rx/Tx method for secondary process
* net/virtio: optimize header reset on any layout
* net/virtio: store IO port info locally
* net/virtio: store PCI operators pointer locally
* net/vmxnet3: fix Rx deadlock
* pci: fix check of mknod
* pmdinfogen: fix endianness with cross-compilation
* pmdinfogen: fix null dereference
* sched: fix crash when freeing port
* usertools: fix active interface detection when binding
* vdev: fix detaching with alias
* vfio: fix file descriptor leak in multi-process
* vhost: allow many vhost-user ports
* vhost: do not GSO when no header is present
* vhost: fix dead loop in enqueue path
* vhost: fix guest/host physical address mapping
* vhost: fix long stall of negotiation
* vhost: fix memory leak

16.11.2
~~~~~~~

* app/testpmd: fix TC mapping in DCB init config
* app/testpmd: fix crash at mbuf pool creation
* app/testpmd: fix exit without freeing resources
* app/testpmd: fix init config for multi-queue mode
* app/testpmd: fix number of mbufs in pool
* app: enable HW CRC strip by default
* crypto/openssl: fix AAD capabilities for AES-GCM
* crypto/openssl: fix AES-GCM capability
* crypto/qat: fix AES-GCM authentication length
* crypto/qat: fix IV zero physical address
* crypto/qat: fix dequeue statistics
* cryptodev: fix API digest length comments
* doc: add limitation of AAD size to QAT guide
* doc: explain zlib dependency for bnx2x
* eal/linux: fix build with glibc 2.25
* eal: fix debug macro redefinition
* examples/ip_fragmentation: fix check of packet type
* examples/l2fwd-crypto: fix AEAD tests when AAD is zero
* examples/l2fwd-crypto: fix packets array index
* examples/l2fwd-crypto: fix padding calculation
* examples/l3fwd-power: fix Rx descriptor size
* examples/l3fwd-power: fix handling no Rx queue
* examples/load_balancer: fix Tx flush
* examples/multi_process: fix timer update
* examples/performance-thread: fix build on FreeBSD
* examples/performance-thread: fix build on FreeBSD 10.0
* examples/performance-thread: fix compilation on Suse 11 SP2
* examples/quota_watermark: fix requirement for 2M pages
* examples: enable HW CRC strip by default
* examples: fix build clean on FreeBSD
* kni: fix build with kernel 4.11
* kni: fix crash caused by freeing mempool
* kni: fix possible memory leak
* mbuf: fix missing includes in exported header
* mk: fix lib filtering when linking app
* mk: fix quoting for ARM mtune argument
* mk: fix shell errors when building with clang
* net/bnx2x: fix transmit queue free threshold
* net/bonding: allow configuring jumbo frames without slaves
* net/bonding: fix updating slave link status
* net/cxgbe: fix possible null pointer dereference
* net/e1000/base: fix multicast setting in VF
* net/ena: cleanup if refilling of Rx descriptors fails
* net/ena: fix Rx descriptors allocation
* net/ena: fix delayed cleanup of Rx descriptors
* net/ena: fix return of hash control flushing
* net/fm10k: fix memory overflow in 32-bit SSE Rx
* net/fm10k: fix pointer cast
* net/i40e/base: fix potential out of bound array access
* net/i40e: add missing 25G link speed
* net/i40e: ensure vector mode is not used with QinQ
* net/i40e: fix TC bitmap of VEB
* net/i40e: fix VF link speed
* net/i40e: fix VF link status update
* net/i40e: fix allocation check
* net/i40e: fix compile error
* net/i40e: fix hash input set on X722
* net/i40e: fix incorrect packet index reference
* net/i40e: fix mbuf alloc failed counter
* net/i40e: fix memory overflow in 32-bit SSE Rx
* net/i40e: fix setup when bulk is disabled
* net/igb: fix VF MAC address setting
* net/igb: fix VF MAC address setting
* net/ixgbe/base: fix build error
* net/ixgbe: fix Rx queue blocking issue
* net/ixgbe: fix TC bandwidth setting
* net/ixgbe: fix VF Rx mode for allmulticast disabled
* net/ixgbe: fix all queues drop setting of DCB
* net/ixgbe: fix memory overflow in 32-bit SSE Rx
* net/ixgbe: fix multi-queue mode check in SRIOV mode
* net/ixgbe: fix setting MTU on stopped device
* net/ixgbevf: set xstats id values
* net/mlx4: fix Rx after mbuf alloc failure
* net/mlx4: fix returned values upon failed probing
* net/mlx4: update link status upon probing with LSC
* net/mlx5: fix Tx when first segment size is too short
* net/mlx5: fix VLAN stripping indication
* net/mlx5: fix an uninitialized variable
* net/mlx5: fix returned values upon failed probing
* net/mlx5: fix reusing Rx/Tx queues
* net/mlx5: fix supported packets types
* net/nfp: fix packet/data length conversion
* net/pcap: fix using mbuf after freeing it
* net/qede/base: fix find zero bit macro
* net/qede: fix FW version string for VF
* net/qede: fix default MAC address handling
* net/qede: fix fastpath rings reset phase
* net/qede: fix missing UDP protocol in RSS offload types
* net/thunderx: fix 32-bit build
* net/thunderx: fix build on FreeBSD
* net/thunderx: fix deadlock in Rx path
* net/thunderx: fix stats access out of bounds
* net/virtio-user: fix address on 32-bit system
* net/virtio-user: fix overflow
* net/virtio: disable LSC interrupt if MSIX not enabled
* net/virtio: fix MSI-X for modern devices
* net/virtio: fix crash when closing twice
* net/virtio: fix link status always being up
* net/virtio: fix link status always down
* net/virtio: fix queue notify
* net/vmxnet3: fix build with gcc 7
* net/vmxnet3: fix queue size changes
* net: fix stripped VLAN flag for offload emulation
* nic_uio: fix device binding at boot
* pci: fix device registration on FreeBSD
* test/cmdline: fix missing break in switch
* test/mempool: free mempool on exit
* test: enable HW CRC strip by default
* vfio: fix disabling INTx
* vfio: fix secondary process start
* vhost: change log levels in client mode
* vhost: fix dequeue zero copy
* vhost: fix false sharing
* vhost: fix fd leaks for vhost-user server mode
* vhost: fix max queues
* vhost: fix multiple queue not enabled for old kernels
* vhost: fix use after free

16.11.3
~~~~~~~

* contigmem: do not zero pages during each mmap
* contigmem: free allocated memory on error
* crypto/aesni_mb: fix HMAC supported key sizes
* cryptodev: fix device stop function
* crypto/openssl: fix HMAC supported key sizes
* crypto/qat: fix HMAC supported key sizes
* crypto/qat: fix NULL authentication hang
* crypto/qat: fix SHA384-HMAC block size
* doc: remove incorrect limitation on AESNI-MB PMD
* doc: remove incorrect limitation on QAT PMD
* eal: fix config file path when checking process
* examples/l2fwd-crypto: fix application help
* examples/l2fwd-crypto: fix option parsing
* examples/l2fwd-crypto: fix padding
* examples/l3fwd: fix IPv6 packet type parse
* examples/qos_sched: fix build for less lcores
* ip_frag: free mbufs on reassembly table destroy
* kni: fix build with gcc 7.1
* lpm: fix index of tbl8
* mbuf: fix debug checks for headroom and tailroom
* mbuf: fix doxygen comment of bulk alloc
* mbuf: fix VXLAN port in comment
* mem: fix malloc element resize with padding
* net/bnxt: check invalid L2 filter id
* net/bnxt: enable default VNIC allocation
* net/bnxt: fix autoneg on 10GBase-T links
* net/bnxt: fix get link config
* net/bnxt: fix reporting of link status
* net/bnxt: fix set link config
* net/bnxt: fix set link config
* net/bnxt: fix vnic cleanup
* net/bnxt: free filter before reusing it
* net/bonding: change link status check to no-wait
* net/bonding: fix number of bonding Tx/Rx queues
* net/bonding: fix when NTT flag updated
* net/cxgbe: fix port statistics
* net/e1000: fix LSC interrupt
* net/ena: fix cleanup of the Tx bufs
* net/enic: fix build with gcc 7.1
* net/enic: fix crash when freeing 0 packet to mempool
* net/fm10k: initialize link status in device start
* net/i40e: add return value checks
* net/i40e/base: fix Tx error stats on VF
* net/i40e: exclude internal packet's byte count
* net/i40e: fix division by 0
* net/i40e: fix ethertype filter for new FW
* net/i40e: fix link down and negotiation
* net/i40e: fix Rx data segment buffer length
* net/i40e: fix VF statistics
* net/igb: fix add/delete of flex filters
* net/igb: fix checksum valid flags
* net/igb: fix flex filter length
* net/ixgbe: fix mirror rule index overflow
* net/ixgbe: fix Rx/Tx queue interrupt for x550 devices
* net/mlx4: fix mbuf poisoning in debug code
* net/mlx4: fix probe failure report
* net/mlx5: fix build with gcc 7.1
* net/mlx5: fix completion buffer size
* net/mlx5: fix exception handling
* net/mlx5: fix inconsistent link status query
* net/mlx5: fix redundant free of Tx buffer
* net/qede: fix chip details print
* net/virtio: do not claim to support LRO
* net/virtio: do not falsely claim to do IP checksum
* net/virtio-user: fix crash when detaching device
* net/virtio: zero the whole memory zone
* net/vmxnet3: fix filtering on promiscuous disabling
* net/vmxnet3: fix receive queue memory leak
* Revert "ip_frag: free mbufs on reassembly table destroy"
* test/bonding: fix memory corruptions
* test/bonding: fix mode 4 names
* test/bonding: fix namespace of the RSS tests
* test/bonding: fix parameters of a balance Tx
* test/crypto: fix overflow
* test/crypto: fix wrong AAD setting
* vhost: fix checking of device features
* vhost: fix guest pages memory leak
* vhost: fix IP checksum
* vhost: fix TCP checksum
* vhost: make page logging atomic

16.11.4
~~~~~~~

* app/testpmd: fix forwarding between non consecutive ports
* app/testpmd: fix invalid port id parameters
* app/testpmd: fix mapping of user priority to DCB TC
* app/testpmd: fix packet throughput after stats reset
* app/testpmd: fix RSS structure initialisation
* app/testpmd: fix topology error message
* buildtools: check allocation error in pmdinfogen
* buildtools: fix icc build
* cmdline: fix compilation with -Og
* cmdline: fix warning for unused return value
* config: fix bnx2x option for armv7a
* cryptodev: fix build with -Ofast
* crypto/qat: fix SHA512-HMAC supported key size
* drivers/crypto: use snprintf return value correctly
* eal/bsd: fix missing interrupt stub functions
* eal: copy raw strings taken from command line
* eal: fix auxv open check for ARM and PPC
* eal/x86: fix atomic cmpset
* examples/ipsec-secgw: fix IPv6 payload length
* examples/ipsec-secgw: fix IP version check
* examples/l2fwd-cat: fix build with PQOS 1.4
* examples/l2fwd-crypto: fix uninitialized errno value
* examples/l2fwd_fork: fix message pool init
* examples/l3fwd-acl: check fseek return
* examples/multi_process: fix received message length
* examples/performance-thread: check thread creation
* examples/performance-thread: fix out-of-bounds sched array
* examples/performance-thread: fix out-of-bounds tls array
* examples/qos_sched: fix uninitialized config
* hash: fix eviction counter
* kni: fix build on RHEL 7.4
* kni: fix build on SLE12 SP3
* kni: fix ethtool build with kernel 4.11
* lpm6: fix compilation with -Og
* mem: fix malloc element free in debug mode
* net/bnxt: fix a bit shift operation
* net/bnxt: fix an issue with broadcast traffic
* net/bnxt: fix a potential null pointer dereference
* net/bnxt: fix interrupt handler
* net/bnxt: fix link handling and configuration
* net/bnxt: fix Rx offload capability
* net/bnxt: fix Tx offload capability
* net/bnxt: set checksum offload flags correctly
* net/bnxt: update status of Rx IP/L4 CKSUM
* net/bonding: fix LACP slave deactivate behavioral
* net/cxgbe: fix memory leak
* net/enic: fix assignment
* net/enic: fix packet loss after MTU change
* net/enic: fix possible null pointer dereference
* net: fix inner L2 length in packet type parser
* net/i40e/base: fix bool definition
* net/i40e: fix clear xstats bug in VF
* net/i40e: fix flexible payload configuration
* net/i40e: fix flow control watermark mismatch
* net/i40e: fix i40evf MAC filter table
* net/i40e: fix mbuf free in vector Tx
* net/i40e: fix memory leak if VF init fails
* net/i40e: fix mirror rule reset when port is closed
* net/i40e: fix mirror with firmware 6.0
* net/i40e: fix packet count for PF
* net/i40e: fix PF notify issue when VF is not up
* net/i40e: fix Rx packets number for NEON
* net/i40e: fix Rx queue interrupt mapping in VF
* net/i40e: fix uninitialized variable
* net/i40e: fix variable assignment
* net/i40e: fix VF cannot forward packets issue
* net/i40e: fix VFIO interrupt mapping in VF
* net/igb: fix memcpy length
* net/igb: fix Rx interrupt with VFIO and MSI-X
* net/ixgbe: fix adding a mirror rule
* net/ixgbe: fix mapping of user priority to TC
* net/ixgbe: fix PF DCB info
* net/ixgbe: fix uninitialized variable
* net/ixgbe: fix VFIO interrupt mapping in VF
* net/ixgbe: fix VF RX hang
* net/mlx5: fix clang build
* net/mlx5: fix clang compilation error
* net/mlx5: fix link speed bitmasks
* net/mlx5: fix probe failure report
* net/mlx5: fix Tx stats error counter definition
* net/mlx5: fix Tx stats error counter logic
* net/mlx5: improve stack usage during link update
* net/nfp: fix RSS
* net/nfp: fix stats struct initial value
* net/pcap: fix memory leak in dumper open
* net/qede/base: fix API return types
* net/qede/base: fix division by zero
* net/qede/base: fix for VF malicious indication
* net/qede/base: fix macros to check chip revision/metal
* net/qede/base: fix number of app table entries
* net/qede/base: fix return code to align with FW
* net/qede/base: fix to use a passed ptt handle
* net/qede: fix icc build
* net/virtio: fix compilation with -Og
* net/virtio: fix mbuf port for simple Rx function
* net/virtio: fix queue setup consistency
* net/virtio: fix Tx packet length stats
* net/virtio: fix untrusted scalar value
* net/virtio: flush Rx queues on start
* net/vmxnet3: fix dereference before null check
* net/vmxnet3: fix MAC address set
* net/vmxnet3: fix memory leak when releasing queues
* pdump: fix possible mbuf leak on failure
* ring: guarantee load/load order in enqueue and dequeue
* test: fix assignment operation
* test/memzone: fix memory leak
* test/pmd_perf: fix crash with multiple devices
* timer: use 64-bit specific code on more platforms
* uio: fix compilation with -Og
* usertools: fix device binding with python 3
* vfio: fix close unchecked file descriptor

16.11.5
~~~~~~~

* app/procinfo: add compilation option in config
* app/testpmd: fix crash of txonly with multiple segments
* app/testpmd: fix flow director filter
* app/testpmd: fix port index in RSS forward config
* app/testpmd: fix port topology in RSS forward config
* bus/pci: fix interrupt handler type
* contigmem: fix build on FreeBSD 12
* crypto/qat: fix allocation check and leak
* crypto/qat: fix null auth algo overwrite
* doc: fix outdated link to IPsec white paper
* eal/ppc: remove the braces in memory barrier macros
* eal/ppc: support sPAPR IOMMU for vfio-pci
* eal: update assertion macro
* eal/x86: use lock-prefixed instructions for SMP barrier
* ethdev: fix data alignment
* ethdev: fix link autonegotiation value
* ethdev: fix missing imissed counter in xstats
* ethdev: fix typo in functions comment
* examples/bond: check mbuf allocation
* examples/exception_path: align stats on cache line
* examples/ip_pipeline: fix timer period unit
* examples/ipsec-secgw: fix corner case for SPI value
* examples/l3fwd-power: fix frequency detection
* examples/l3fwd-power: fix Rx without interrupt
* examples/vhost: fix sending ARP packet to self
* examples/vhost: fix startup check
* igb_uio: fix IRQ disable on recent kernels
* igb_uio: fix MSI-X IRQ assignment with new IRQ function
* igb_uio: switch to new irq function for MSI-X
* keepalive: fix state alignment
* kni: fix build with kernel 4.15
* lpm: fix ARM big endian build
* malloc: fix end for bounded elements
* malloc: protect stats with lock
* mbuf: cleanup function to get last segment
* mbuf: fix NULL freeing when debug enabled
* mem: fix mmap error check on huge page attach
* memzone: fix leak on allocation error
* mk: fix external build
* mk: support renamed Makefile in external project
* net/bnxt: fix broadcast cofiguration
* net/bnxt: fix group info usage
* net/bnxt: fix headroom initialization
* net/bnxt: fix link speed setting with autoneg off
* net/bnxt: fix Rx checksum flags
* net/bnxt: fix size of Tx ring in HW
* net/bnxt: parse checksum offload flags
* net/bnxt: support new PCI IDs
* net/bonding: check error of MAC address setting
* net/bonding: fix activated slave in 8023ad mode
* net/bonding: fix setting slave MAC addresses
* net/e1000: fix mailbox interrupt handler
* net/e1000: fix VF Rx interrupt enabling
* net/ena: do not set Tx L4 offloads in Rx path
* net/enic: fix crash due to static max number of queues
* net/fm10k: fix logical port delete
* net/i40e: add debug logs when writing global registers
* net/i40e: add warnings when writing global registers
* net/i40e/base: fix compile issue for GCC 6.3
* net/i40e/base: fix link LED blink
* net/i40e/base: fix NVM lock
* net/i40e: check multi-driver option parsing
* net/i40e: fix ARM big endian build
* net/i40e: fix flag for MAC address write
* net/i40e: fix flow director Rx resource defect
* net/i40e: fix interrupt conflict when using multi-driver
* net/i40e: fix multiple driver support issue
* net/i40e: fix Rx interrupt
* net/i40e: fix VF reset stats crash
* net/i40e: fix VF Rx interrupt enabling
* net/i40e: fix VLAN offload setting
* net/i40e: fix VSI MAC filter on primary address change
* net/i40e: implement vector PMD for altivec
* net/igb: fix Tx queue number assignment
* net/ixgbe/base: add media type of fixed fiber
* net/ixgbe: fix ARM big endian build
* net/ixgbe: fix mailbox interrupt handler
* net/ixgbe: fix max queue number for VF
* net/ixgbe: fix reset error handling
* net/ixgbe: fix the failure of number of Tx queue check
* net/ixgbe: fix VF Rx interrupt enabling
* net/ixgbe: improve link state check on VF
* net/mlx5: fix deadlock of link status alarm
* net/mlx5: fix missing RSS capability
* net/mlx5: fix MTU update
* net/nfp: fix CRC strip check behaviour
* net/nfp: fix jumbo settings
* net/nfp: fix MTU settings
* net/pcap: fix the NUMA id display in logs
* net/qede/base: fix VF LRO tunnel configuration
* net/qede: fix clearing of queue stats
* net/qede: fix few log messages
* net/qede: fix MTU set and max Rx pkt len usage
* net/qede: fix to reject config with no Rx queue
* net/szedata2: fix check of mmap return value
* net/thunderx: fix multi segment Tx function return
* net/vhost: fix log messages on create/destroy
* net/virtio: fix incorrect cast
* net/virtio: fix mbuf data offset for simple Rx
* net/virtio: fix memory leak when reinitializing device
* net/virtio: fix queue flushing with vector Rx enabled
* net/virtio: fix resuming port with Rx vector path
* net/virtio: fix Rx and Tx handler selection for ARM32
* net/virtio: fix typo in function name
* net/virtio: fix vector Rx flushing
* net/virtio-user: fix start with kernel vhost
* pdump: fix error check when creating/canceling thread
* pmdinfogen: fix cross compilation for ARM big endian
* test/crypto: fix missing include
* test/memzone: fix freeing test
* test/memzone: fix NULL freeing
* test/memzone: fix wrong test
* test/memzone: handle previously allocated memzones
* test/pmd_perf: declare variables as static
* test: register test as failed if setup failed
* test/reorder: fix memory leak
* test/ring_perf: fix memory leak
* test/table: fix memory leak
* test/table: fix uninitialized parameter
* test/timer_perf: fix memory leak
* usertools/devbind: remove unused function
* vfio: fix enabled check on error
* vhost: do not take lock on owner reset
* vhost: fix crash
* vhost: fix dequeue zero copy with virtio1
* vhost: fix error code check when creating thread
* vhost: fix mbuf free
* vhost: protect active rings from async ring changes

16.11.6
~~~~~~~

* vhost: add support for non-contiguous indirect descs tables (fixes CVE-2018-1059)
* vhost: check all range is mapped when translating GPAs (fixes CVE-2018-1059)
* vhost: ensure all range is mapped when translating QVAs (fixes CVE-2018-1059)
* vhost: handle virtually non-contiguous buffers in Rx (fixes CVE-2018-1059)
* vhost: handle virtually non-contiguous buffers in Rx-mrg (fixes CVE-2018-1059)
* vhost: handle virtually non-contiguous buffers in Tx (fixes CVE-2018-1059)
* vhost-user: fix deadlock in case of NUMA realloc

16.11.7
~~~~~~~

* app/crypto-perf: fix parameters copy
* app/testpmd: fix burst stats reporting
* app/testpmd: fix command token
* app/testpmd: fix forward ports Rx flush
* app/testpmd: fix forward ports update
* app/testpmd: fix slave port detection
* app/testpmd: fix synchronic port hotplug
* app/testpmd: fix valid ports prints
* bus/pci: fix size of driver name buffer
* crypto/zuc: do not set default op status
* crypto/zuc: remove unnecessary check
* doc: fix a typo in the EAL guide
* drivers/net: fix icc deprecated parameter warning
* drivers/net: fix link autoneg value for virtual PMDs
* eal: declare trace buffer at top of own block
* eal: explicit cast in rwlock functions
* eal: explicit cast of builtin for bsf32
* eal: explicit cast of core id when getting index
* eal: fix casts in random functions
* eal: fix typo in doc of pointer offset macro
* eal/ppc: remove braces in SMP memory barrier macro
* eal: remove unused path pattern
* eal: support strlcpy function
* ethdev: explicit cast of buffered Tx number
* ethdev: explicit cast of queue count return
* ethdev: fix queue start
* ethdev: fix string length in name comparison
* ethdev: fix type and scope of variables in Rx burst
* ethdev: improve doc for name by port ID API
* examples/exception_path: limit core count to 64
* examples/performance-thread: fix return type of threads
* hash: explicit casts for truncation in CRC32c
* hash: fix comment for lookup
* hash: move stack declaration at top of CRC32c function
* ip_frag: fix double free of chained mbufs
* ip_frag: fix some debug logs
* kni: fix build on RHEL 7.5
* kvargs: fix syntax in comments
* mbuf: avoid integer promotion in prepend/adj/chain
* mbuf: explicit cast of headroom on reset
* mbuf: explicit cast of size on detach
* mbuf: explicit casts of reference counter
* mbuf: fix reference counter integer promotion
* mbuf: fix Tx checksum offload API doc
* mbuf: fix type of private size in detach
* mempool: fix leak when no objects are populated
* mempool: fix virtual address population
* memzone: fix size on reserving biggest memzone
* net/bnx2x: do not cast function pointers as a policy
* net/bnx2x: fix for PCI FLR after ungraceful exit
* net/bnx2x: fix KR2 device check
* net/bnx2x: fix memzone name overrun
* net/bnxt: avoid freeing memzone multiple times
* net/bnxt: fix endianness of flag
* net/bnxt: fix mbuf data offset initialization
* net/bnxt: fix Rx checksum flags
* net/bnxt: fix Rx checksum flags for tunnel frames
* net/bnxt: fix Rx drop setting
* net/bnxt: fix Rx mbuf and agg ring leak in dev stop
* net/bonding: clear started state if start fails
* net/bonding: export mode 4 slave info routine
* net/bonding: fix setting VLAN ID on slave ports
* net/enic: allocate stats DMA buffer upfront during probe
* net/enic: fix crash on MTU update with non-setup queues
* net: explicit cast in L4 checksum
* net: explicit cast of IP checksum to 16-bit
* net: explicit cast of multicast bit clearing
* net: explicit cast of protocol in IPv6 checksum
* net/i40e: fix failing to disable FDIR Tx queue
* net/i40e: fix intr callback unregister by adding retry
* net/i40e: fix link status update
* net/i40e: fix link update no wait
* net/i40e: fix shifts of signed values
* net/ixgbe: fix DCB configuration
* net/ixgbe: fix intr callback unregister by adding retry
* net/ixgbe: fix too many interrupts
* net/mlx5: fix ARM build
* net/mlx5: fix double free on error handling
* net/mlx5: fix resource leak in case of error
* net: move stack variable at top of VLAN strip function
* net/nfp: fix assigning port id in mbuf
* net/nfp: fix barrier location
* net/nfp: fix mbufs releasing when stop or close
* net/nfp: fix memcpy out of source range
* net/qede: fix alloc from socket 0
* net/qede: fix strncpy
* net/qede: fix unicast filter routine return code
* net/qede: replace strncpy by strlcpy
* net/szedata2: fix format string for PCI address
* net/szedata2: fix total stats
* net/thunderx: fix MTU configuration for jumbo packets
* net/vhost: initialise device as inactive
* net/virtio-user: fix hugepage files enumeration
* net/vmxnet3: keep link state consistent
* net/vmxnet3: set the queue shared buffer at start
* Revert "vhost: fix device cleanup at stop"
* spinlock/x86: move stack declaration before code
* test/distributor: fix return type of thread function
* test: fix memory flags test for low NUMA nodes number
* test/mempool: fix autotest retry
* test/pipeline: fix return type of stub miss
* test/pipeline: fix type of table entry parameter
* test/reorder: fix freeing mbuf twice
* vhost: check cmsg not null
* vhost: fix compilation issue when vhost debug enabled
* vhost: fix dead lock on closing in server mode
* vhost: fix device cleanup at stop
* vhost: fix log macro name conflict
* vhost: fix offset while mmaping log base address
* vhost: fix realloc failure
* vhost: fix typo in comment
* vhost: improve dirty pages logging performance

16.11.8
~~~~~~~

* app/testpmd: fix DCB config
* app/testpmd: fix VLAN TCI mask set error for FDIR
* crypto/qat: fix checks for 3GPP algo bit params
* doc: fix bonding command in testpmd
* eal: fix bitmap documentation
* eal: fix return codes on thread naming failure
* eal/linux: fix invalid syntax in interrupts
* ethdev: check queue stats mapping input arguments
* ethdev: fix a doxygen comment for port allocation
* ethdev: fix queue statistics mapping documentation
* examples/exception_path: fix out-of-bounds read
* examples/ipsec-secgw: fix bypass rule processing
* examples/ipsec-secgw: fix IPv4 checksum at Tx
* examples/l3fwd: remove useless include
* examples/multi_process: build l2fwd_fork app
* hash: fix a multi-writer race condition
* hash: fix doxygen of return values
* hash: fix key slot size accuracy
* hash: fix multiwriter lock memory allocation
* kni: fix build on RHEL 7.5
* kni: fix build with gcc 8.1
* kni: fix crash with null name
* maintainers: claim maintainership for ARM v7 and v8
* maintainers: update for Mellanox PMDs
* mbuf: fix typo in IPv6 macro comment
* mk: fix permissions when using make install
* net/bnx2x: fix FW command timeout during stop
* net/bnxt: check access denied for HWRM commands
* net/bnxt: fix close operation
* net/bnxt: fix HW Tx checksum offload check
* net/bnxt: fix incorrect IO address handling in Tx
* net/bnxt: fix RETA size
* net/bnxt: fix Rx ring count limitation
* net/bnxt: fix Tx with multiple mbuf
* net/bonding: do not clear active slave count
* net/bonding: fix MAC address reset
* net/bonding: fix race condition
* net/cxgbe/base: update flash part information
* net/cxgbe: fix init failure due to new flash parts
* net/ena: change memory type
* net/ena: check pointer before memset
* net/ena: fix GENMASK_ULL macro
* net/ena: fix SIGFPE with 0 Rx queue
* net/ena: set link speed as none
* net/enic: do not overwrite admin Tx queue limit
* net/i40e: do not reset device info data
* net/i40e: fix check of flow director programming status
* net/i40e: fix link speed
* net/i40e: fix shifts of 32-bit value
* net/i40e: revert fix of flow director check
* net/i40e: workaround performance degradation
* net/ixgbe: fix mask bits register set error for FDIR
* net/ixgbe: fix tunnel id format error for FDIR
* net/ixgbe: fix tunnel type set error for FDIR
* net/nfp: fix field initialization in Tx descriptor
* net/null: add MAC address setting fake operation
* net/pcap: fix multiple queues
* net/qede/base: fix GRC attention callback
* net/qede: fix default extended VLAN offload config
* net/qede: fix MAC address removal failure message
* net: rename u16 to fix shadowed declaration
* net/thunderx: avoid sq door bell write on zero packet
* net/thunderx: fix build with gcc optimization on
* Revert "net/i40e: fix packet count for PF"
* test/bonding: assign non-zero MAC to null devices
* test/crypto: fix device id when stopping port
* test: fix EAL flags autotest on FreeBSD
* test: fix uninitialized port configuration
* test/hash: fix multiwriter with non consecutive cores
* test/hash: fix potential memory leak
* test/virtual_pmd: add MAC address setting fake op
* vhost: fix missing increment of log cache count

16.11.9
~~~~~~~

* acl: forbid rule with priority zero
* app/testpmd: fix csum parse-tunnel command invocation
* app/testpmd: fix displaying RSS hash functions
* app/testpmd: fix duplicate exit
* app/testpmd: fix L4 length for UDP checksum
* app/testpmd: optimize mbuf pool allocation
* build: enable ARM NEON flag when __aarch64__ defined
* bus/pci: fix allocation of device path
* config: enable more than 128 cores for arm64
* config: make AVX and AVX512 configurable
* doc: add VFIO in ENA guide
* doc: fix formatting in IP reassembly app guide
* doc: fix NUMA library name in Linux guide
* doc: fix typo in testpmd guide
* doc: fix wrong usage of bind command
* eal: fix build
* eal: fix build with gcc 9.0
* eal: fix build with -O1
* eal: introduce rte version of fls
* eal/linux: fix memory leak of logid
* eal/linux: handle UIO read failure in interrupt handler
* eal: use correct data type for bitmap slab operations
* ethdev: fix doxygen comment to be with structure
* ethdev: fix invalid configuration after failure
* ethdev: fix queue start and stop
* examples/ipv4_multicast: fix leak of cloned packets
* examples/vhost: remove unnecessary constant
* fix dpdk.org URLs
* fix global variable issues
* hash: fix key store element alignment
* hash: remove unnecessary pause
* igb_uio: fix unexpected removal for hot-unplug
* igb_uio: issue FLR during open and release of device file
* igb_uio: remove device reset in open
* igb_uio: remove device reset in release
* ip_frag: check fragment length of incoming packet
* ip_frag: fix overflow in key comparison
* ip_frag: use key length for key comparison
* kni: fix build on CentOS 7.4
* kni: fix build on Linux < 3.14
* kni: fix build on Linux 4.19
* kni: fix build on Suse 12 SP3
* kni: fix kernel FIFO synchronization
* kni: fix possible uninitialized variable
* kni: fix SLE version detection
* kvargs: fix processing a null list
* mk: disable gcc AVX512F support
* net/bnx2x: fix call to link handling periodic function
* net/bnx2x: fix logging to include device name
* net/bnx2x: fix to add PHY lock
* net/bnx2x: fix to disable further interrupts
* net/bnx2x: fix VF link state update
* net/bnxt: fix uninitialized pointer access in Tx
* net/bnxt: reduce polling interval for valid bit
* net/bnxt: remove excess log messages
* net/bnxt: set a VNIC as default only once
* net/bnxt: set MAC filtering as outer for non tunnel frames
* net/bonding: do not ignore RSS key on device config
* net/bonding: fix crash when stopping mode 4 port
* net/bonding: fix Rx slave fairness
* net/bonding: reduce slave starvation on Rx poll
* net/bonding: stop and deactivate slaves on stop
* net/bonding: support matching QinQ ethertype
* net/bonding: use evenly distributed default RSS RETA
* net/e1000/base: fix uninitialized variable
* net/e1000: do not error out if Rx drop enable is set
* net/ena: fix passing RSS hash to mbuf
* net/enic: add dependency on librte_kvargs
* net/enic: add devarg to specify ingress VLAN rewrite mode
* net/enic: do not use non-standard integer types
* net/enic: set Rx VLAN offload flag for non-stripped packets
* net: fix build with pedantic
* net/i40e/base: correct global reset timeout calculation
* net/i40e/base: fix partition id calculation for X722
* net/i40e/base: gracefully clean the resources
* net/i40e/base: properly clean resources
* net/i40e: enable loopback function for X722 MAC
* net/i40e: fix send admin queue command before init
* net/i40e: fix X710 Rx after reading some registers
* net/i40e: keep promiscuous on if allmulticast is enabled
* net/i40e: update Tx offload mask
* net/igb: update Tx offload mask
* net/ixgbe: fix maximum wait time in comment
* net/ixgbe: fix TDH register write
* net/ixgbe: update Tx offload mask
* net/ixgbevf: fix link state
* net/ixgbe: wait longer for link after fiber MAC setup
* net/mlx5: fix build on PPC64
* net/nfp: fix live MAC changes not supported
* net/nfp: fix mbuf flags with checksum good
* net/nfp: fix RSS
* net/thunderx: fix Tx desc corruption in scatter-gather mode
* net/vhost: fix parameters string
* net/virtio: add missing supported features
* net/virtio: register/unregister intr handler on start/stop
* net/virtio-user: do not reset owner when driver resets
* net/virtio-user: fix typo in error message
* pci: fix parsing of address without function number
* test/crypto: fix number of queue pairs
* test/hash: fix bucket size in perf test
* test/hash: fix build
* test/kni: check module dependency
* test/reorder: fix out of bound access
* usertools: check for lspci dependency
* version: 16.11.9-rc1
* version: 16.11.9-rc2
* vfio: fix build
* vfio: fix build on old kernel
* vhost: fix corner case for enqueue operation
* vhost: fix payload size of reply
* vhost: remove unneeded null pointer check
* vhost-user: drop connection on message handling failures
* vhost-user: fix false negative in handling user messages

16.11.10 Release Notes
----------------------

16.11.10 Fixes
~~~~~~~~~~~~~~

* vhost: validate virtqueue size
* vhost: add number of fds to vhost-user messages
* vhost: fix possible denial of service by leaking FDs - CVE-2019-14818
* vhost: fix possible denial of service on SET_VRING_NUM - CVE-2019-14818

16.11.10 Validation
~~~~~~~~~~~~~~~~~~~

* Tested with two testpmd instances, one with Vhost PMD, the other with Virtio-user
  PMD. Intialization goes well, and packets flow.

16.11.11 Release Notes
----------------------

16.11.11 Fixes
~~~~~~~~~~~~~~

* vhost: fix vring requests validation broken if no FD

16.11.11 Validation
~~~~~~~~~~~~~~~~~~~

* virtio/vhost regression tests by Intel:
  * http://doc.dpdk.org/dts/test_plans/virtio_pvp_regression_test_plan.html
  * http://doc.dpdk.org/dts/test_plans/vhost_dequeue_zero_copy_test_plan.html
  * http://doc.dpdk.org/dts/test_plans/vm2vm_virtio_pmd_test_plan.html
