# ASCTools Overview
---
## ASCTools provides quick and simple tools for performing repetitive actions on .ASC raster files.

Source code can be found in /src/  
Prebuilt binaries can be found in /bin/ for **MacOS** and **Linux**.  

---

Additionally, tools are being developed for working with [LSS](https://www.dtmsoftware.com/) proprietary files.  
These files store point/line information and the tools aim to ease use of these files outside of the program.

---

## Commands

| Command         | Usage                                                                                 |
|-----------------|---------------------------------------------------------------------------------------|
| `asc2csv`       | `Usage: asc2csv <input.asc>`                                                         |
| `asc2las`       | `Usage: asc2las <input.asc> [-elev_rgb]` (Optional generation of rgb values based on elevation) |
| `asc2tif`       | `Usage: asc2tif <input.asc> <epsg_code>`                                             |
| `lssinfo`       | `Usage: lssinfo <input.00{x}>`                                                       |
| `lss2csv`       | `Usage: lss2csv <input.00{x}>`                                                       |
| `lss2boundary`  | `Usage: lss2boundary <input.00{x}>`                                                  |
| `lss2json`      | `Usage: lss2json <input.00{x}>`                                                      |
| `lss2dxflines`  | `Usage: lss2dxflines <input.00{x}>`                                                  |
| `lss2las`       | `Usage: lss2las <input.00{x}> [-elev_rgb]` (Optional generation of rgb values based on elevation) |

