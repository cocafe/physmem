## physmem

A command line utility to read/write physical memory on Windows via asmmap64 driver. Run with **administrative privileges**.

Thanks @**[Hyatice](https://github.com/Hyatice)** and @**[ciphray](https://github.com/ciphray)** for helps.



#### ⚠ Warning ⚠

Reading or writing some locations of physical memory can cause data corruption, crash, or any unexpected behaviors. Beware for endianness.



#### Disclaimer

This power users program is written by a n00b. I have no responsibility for any damage caused by using this program. It has no warranty absolutely. Use on your on risk. 👻



#### Usage

This program utilizes some IOCTL APIs provided by `asmmap64.sys` to implement related features.

Please make sure `asmmap64.sys` is in the same folder with `physmem.exe`. 

```
Usage:
       physmem.exe [options] read8    <addr>
       physmem.exe [options] read16   <addr>
       physmem.exe [options] read32   <addr>
       physmem.exe [options] read64   <addr>
       physmem.exe [options] readblk  <addr> <bytes>
       physmem.exe [options] write8   <addr> <value>
       physmem.exe [options] write16  <addr> <value>
       physmem.exe [options] write32  <addr> <value>
       physmem.exe [options] write64  <addr> <value>
       physmem.exe [options] writeblk <addr> <bytes> <byte0> <byte1> ...
       physmem.exe [options] driver install
       physmem.exe [options] driver remove
Options:
       -h              this help text
       -v              verbose print
       -s              no readback after writing
       -m <bytes>      mmap size, default: 8
       -x              always remove asmmap64 driver on exit
       -f              force remove driver for command "driver remove"
       -C              hexdump style print

```

```shell
# read 1 byte with hexdump format
physmem.exe -C read8 0xfed159a0

# read 8 bytes
physmem.exe read64 0xfed159a0

# read 8 bytes block and display in hexdump format
physmem.exe readblk 0xfed159a0 8

# write 8 bytes
physmem.exe write64 0xfed159a0 0x0042820000FE8200

# write 8 bytes silently
physmem.exe -s write64 0xfed159a0 0x0042820000FE8200

# write 8 bytes block
physmem.exe writeblk 0xfed159a0 8 00 82 FE 00 00 82 42 00
```



#### Build

Build with CMAKE on MinGW64.



#### License

```C
/* 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
```



#### References

- https://github.com/namazso/physmem_drivers
- https://github.com/branw/DonkeyKom
- https://github.com/waryas/EUPMAccess

