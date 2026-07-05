# PCI Bus Scanner & AHCI (SATA) Storage Driver

This module documents the low-level hardware detection of the PCI bus and the implementation of the memory-mapped AHCI (SATA) disk driver in the Keira Kernel.

---

## 1. PCI Configuration Space and Scanning

Peripheral Component Interconnect (PCI) is the standard bus protocol used to detect and communicate with motherboard peripherals.

### Address and Data Registers
The CPU interacts with the PCI host controller via two 32-bit I/O ports:
- **`0xCF8` (CONFIG_ADDRESS)**: Written to specify the target device configuration register.
- **`0xCFC` (CONFIG_DATA)**: Read or written to access the target configuration register value.

### Address Layout
To read a register, the 32-bit address written to `0xCF8` is structured as:
```
31           30-24     23-16      15-11      10-8      7-2       1-0
[ Enable ] [ Reserved ] [ Bus ] [ Device ] [ Func ] [ Reg Offset ] [ 00 ]
```

### Probing and Mapping
The PCI Scanner scans through all buses `0..8`, slots `0..32`, and functions `0..8` on boot:
1. Queries offset `0` to read the Vendor ID. A value of `0xFFFF` or `0x0000` indicates no device is present.
2. Reads offset `0x0C` (Header Type) to determine if the device is multi-functional.
3. Reads Class Code (offset `0x08`, bits 24-31) and Subclass (offset `0x08`, bits 16-23) to identify device types (e.g. `0x01/0x06` for SATA Controller).
4. Reads BAR5 (offset `0x24`) which contains the memory base address (ABAR) of the storage controller.
5. Populates the global `PCI_DEVICES` array.

---

## 2. AHCI SATA Controller Driver

The AHCI (Advanced Host Controller Interface) driver is a memory-mapped storage driver that interacts with SATA drives.

### ABAR Registers (BAR5)
The SATA controller's configuration registers are mapped to physical memory (BAR5).
1. **Paging**: During boot, the VMM identity-maps the BAR5 physical address range (`PAGE_WRITABLE`) so the kernel can access HBA registers.
2. **GHC (Global Host Control)**:
   - Sets the `AE` (AHCI Enable) bit `31`.
   - Triggers HBA Reset by setting `HR` bit `0` and waiting for it to clear with a bare-metal microsecond busy loop delay (`io_delay`).

### Port Probing, Initialization & Signatures
The driver reads the `PI` (Ports Implemented) register to check which of the 32 HBA ports are active:
- Each port's registers start at offset `0x100 + (port * 0x80)`.
- Polls `PxSSTS` (SATA Status) to wait for link establishment (`DET == 3` and `IPM == 1`).
- Allocates physical frames for the Command List Base (`PxCLB`), Received FIS Base (`PxFB`), and Command Table Base (`PxCTB`).
- Identity-maps these tables and registers their physical addresses with the controller.
- Starts the command engine (`ST`) and FIS receive (`FRE`) bits.
- Reads `PxSIG` (Signature) to identify the device type:
  - **`0x00000101`**: SATA Hard Disk (ATA)
  - **`0xEB140101`**: CD-ROM Drive (ATAPI)

### DMA Sector Transfers
For reading and writing sectors, the driver constructs DMA command packets:
1. **Command Header**: Sets PRDT length to `1` and points command table address to `PxCTB`.
2. **CFIS**: Populates Host-to-Device Register FIS (type `0x27`) with Command byte `0x25` (READ DMA EXT) or `0x35` (WRITE DMA EXT) and targets the sector index (LBA48).
3. **PRDT (Physical Region Descriptor Table)**: Configures the buffer address pointing to a page-aligned physical sector buffer (`SECTOR_BUF_PHYS`) and size `511` (512 bytes).
4. **Command Issue**: Triggers bit `0` in `PxCI` (Command Issue) and polls for completion, checking Task File Data (`PxTFD`) for error status.

### BlockDevice Registration
Upon finding a SATA Disk, the driver instantiates an `AhciBlockDevice` struct and registers it as `ahci0` with the block device registry (`BlockDevice` trait), mapping virtual buffer reads/writes through the page-aligned DMA sector buffer.

---

## 3. Interactive Shell Commands

### `pci` Command
The interactive `pci` command reads the `PCI_DEVICES` list and prints a formatted table of all detected peripherals:
```
BUS   SLOT  FUNC  VENDOR  DEVICE  CLASS TYPE
0     1     0     8086    1237    Bridge Device
0     2     0     1013    00B0    Display Controller (VGA)
0     3     0     8086    2922    SATA Controller (AHCI)
```

### `drives` Command
The updated `drives` command lists `ahci0` and identifies its type as `"SATA Disk"` with its total user addressable sector size:
```
NAME      TYPE      SIZE (KB)   STATUS
ide0      IDE Disk  32768       Mounted
ahci0     SATA Disk 10240       Unmounted
```
