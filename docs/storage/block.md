# IDE Block Storage Driver

This module covers the hardware-level communication with IDE storage devices, detailing the LBA28 command registers and sector reading/writing.

---

## 1. IDE Primary Master Probing

The IDE disk driver interacts with the default IDE controller:
- **Port Ranges**:
  - **Data/Command Port**: `0x1F0` - `0x1F7`
  - **Control Port**: `0x3F6`
- During device probing, the driver queries the primary master drive by writing to the drive selection register `0x1F6`. It executes the IDE `IDENTIFY` command (`0xEC`) to confirm a disk is connected, reading status registers to identify sector capacity and cylinder metrics.

---

## 2. LBA28 Addressing Commands

Disk reading and writing are performed using Logical Block Addressing (LBA) with 28-bit addresses (LBA28):
- Sector count is written to port `0x1F2`.
- LBA bits [7:0] are written to `0x1F3`.
- LBA bits [15:8] are written to `0x1F4`.
- LBA bits [23:16] are written to `0x1F5`.
- LBA bits [27:24] combined with the drive selector are written to `0x1F6`.

---

## 3. Read & Write Sector Implementation

The driver implements two primary sector I/O functions:

### Sector Read
1. Sends the LBA coordinates to the registers.
2. Sends the `READ SECTORS` command (`0x20`) to port `0x1F7`.
3. Polls the IDE status register until the BSY (Busy, bit 7) bit is cleared and the DRQ (Data Request, bit 3) bit is set.
4. Reads 256 words (512 bytes) from the Data Port `0x1F0` into the memory buffer.

### Sector Write
1. Sends the LBA coordinates to the registers.
2. Sends the `WRITE SECTORS` command (`0x30`) to port `0x1F7`.
3. Polls until BSY is cleared and DRQ is set.
4. Writes 256 words (512 bytes) from the memory buffer to the Data Port `0x1F0`.
5. Sends cache flush commands if supported.
