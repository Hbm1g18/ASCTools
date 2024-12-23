#include <stdio.h>
#include <string.h>

void print_help() {
    printf("| Command         | Usage                                                                                             |\n");
    printf("|-----------------|---------------------------------------------------------------------------------------------------|\n");
    printf("| `asc2csv`       | `Usage: asc2csv <input.asc>`                                                                      |\n");
    printf("| `asc2las`       | `Usage: asc2las <input.asc> [-elev_rgb]` (Optional generation of rgb values based on elevation)   |\n");
    printf("| `asc2tif`       | `Usage: asc2tif <input.asc> <epsg_code>`                                                          |\n");
    printf("| `asc2pointgrid` | `Usage: asc2pointgrid <input.asc> [-spacing {x}]`                                                 |\n");
    printf("|                 |   Outputs a dxf file with spot levels plotted as a grid. Optional spacing arg                     |\n");
    printf("| `lssinfo`       | `Usage: lssinfo <input.00{x}>`                                                                    |\n");
    printf("| `lss2csv`       | `Usage: lss2csv <input.00{x}>`                                                                    |\n");
    printf("| `lss2boundary`  | `Usage: lss2boundary <input.00{x}>`                                                               |\n");
    printf("| `lss2json`      | `Usage: lss2json <input.00{x}>`                                                                   |\n");
    printf("| `lss2dxflines`  | `Usage: lss2dxflines <input.00{x}> [--one-code {x}] [--list-codes {x},{y}{z}]`                    |\n");
    printf("|                 | [--one-code] generates a dxf output with only that feature code present.                          |\n");
    printf("|                 | [--list-codes] generates a dxf output from a comma delimited list of feature codes.               |\n");
    printf("| `lss2las`       | `Usage: lss2las <input.00{x}> [-elev_rgb]` (Optional generation of rgb values based on elevation) |\n");
    printf("| `lss2web`       |  `Usage: lss2web <input.00{x}> [-ge] [-points]`                                                   |\n");
    printf("|                 |  `Enable Google Earth basemap tiles and include all points from survey on map`                    |\n");
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help();
            return 0;
        } else {
            printf("Invalid argument. Use -h or --help for usage information.\n");
            return 1;
        }
    }

    print_help();
    return 0;
}
