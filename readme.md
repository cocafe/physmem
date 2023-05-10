## physmem

A command line utility to read/write physical memory on Windows via **vulnerable** asmmap64 or inpoutx64 driver.

**Administrative privileges** is required to install the kernel driver.

Thanks @**[Hyatice](https://github.com/Hyatice)** and @**[ciphray](https://github.com/ciphray)** for helps.



### ⚠ Warning ⚠

- Reading or writing some locations of physical memory can cause data corruption, crash, or any unexpected behaviors.
- If `asmmap64/inpoutx64` driver is not removed from system, calling its APIs does **NOT** require promoted privileges to read/write arbitrary memory location. **Use with cautions!!!**
  - Uninstall driver from runtime after RW memory is highly recommended
- Beware for endianness.



### Disclaimer

This power users program is written by a n00b. I have no responsibility for any damage caused by using this program. It has no warranty absolutely. Use on your on risk. 👻



### Usage

This program utilizes APIs provided by `asmmap64.sys/inpoutx64.sys` to implement related features.

Please make sure required files are in the same folder with `physmem.exe`. 

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

```shell
# for inpoutx64 version, the driver needs to be installed/removed manually before/after RW memory
physmem.exe driver install
physmem.exe driver remove
```


### Build

Build with CMAKE on MinGW64.



### Issues

- `asmmap64` cannot remove from system via `driver remove` for now until reboot
  - for security, use command "[ntdrvldr](https://github.com/iceboy233/ntdrvldr) **-u -n asmmap64 1**" to stop and remove driver from runtime **instantly**

- `asmmap64/inpoutx64` may be blocked on `Windows 11 22h2` and later, set `VulnerableDriverBlocklistEnable` = 0 in `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\CI\Config` to remove this limitation. 

  


### License

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



### References

- https://github.com/namazso/physmem_drivers
- https://github.com/branw/DonkeyKom
- https://github.com/waryas/EUPMAccess
- https://github.com/LibreHardwareMonitor/LibreHardwareMonitor
