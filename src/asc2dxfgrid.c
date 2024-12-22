#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.asc> -spacing <spacing_value>\n", argv[0]);
        return 1;
    }

    char *input_file = argv[1];
    float spacing_value = 0.0;

    if (argc == 4 && strcmp(argv[2], "-spacing") == 0) {
        spacing_value = atof(argv[3]);
        if (spacing_value <= 0) {
            fprintf(stderr, "Invalid spacing value. It must be greater than 0.\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Usage: %s <input.asc> -spacing <spacing_value>\n", argv[0]);
        return 1;
    }

    char output_file[256];
    strncpy(output_file, input_file, sizeof(output_file) - 5);
    output_file[sizeof(output_file) - 5] = '\0';
    char *dot = strrchr(output_file, '.');
    if (dot) *dot = '\0';
    strcat(output_file, ".dxf");

    FILE *fp = fopen(input_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error opening input file '%s': %s\n", input_file, strerror(errno));
        return 1;
    }

    char line[255];
    int nrows_value = 0, ncols_value = 0, nodata_int_value = -9999;
    float xllcorner_value = 0.0, yllcorner_value = 0.0, cellsize_value = 0.0, nodata_float_value = -9999.0;

    for (int i = 0; i < 6; i++) {
        fgets(line, sizeof(line), fp);
        if (strstr(line, "nrows")) sscanf(line, "%*s %d", &nrows_value);
        else if (strstr(line, "ncols")) sscanf(line, "%*s %d", &ncols_value);
        else if (strstr(line, "xllcorner")) sscanf(line, "%*s %f", &xllcorner_value);
        else if (strstr(line, "yllcorner")) sscanf(line, "%*s %f", &yllcorner_value);
        else if (strstr(line, "cellsize")) sscanf(line, "%*s %f", &cellsize_value);
        else if (strstr(line, "nodata_value")) sscanf(line, "%*s %d", &nodata_int_value);
    }

    nodata_float_value = (float)nodata_int_value;

    printf("Header processed, generating '%s'\n", output_file);

    FILE *dxf_file = fopen(output_file, "w");
    if (dxf_file == NULL) {
        fprintf(stderr, "Error creating output file '%s': %s\n", output_file, strerror(errno));
        fclose(fp);
        return 1;
    }

    fprintf(dxf_file, "0\nSECTION\n2\nHEADER\n0\nENDSEC\n");
    fprintf(dxf_file, "0\nSECTION\n2\nTABLES\n0\nENDSEC\n");
    fprintf(dxf_file, "0\nSECTION\n2\nBLOCKS\n");
    fprintf(dxf_file, "0\nBLOCK\n8\n0\n2\nCrossBlock\n70\n0\n");
    fprintf(dxf_file, "10\n0.0\n20\n0.0\n30\n0.0\n");
    fprintf(dxf_file, "0\nLINE\n8\n0\n10\n-0.5\n20\n0.0\n30\n0.0\n11\n0.5\n21\n0.0\n31\n0.0\n");
    fprintf(dxf_file, "0\nLINE\n8\n0\n10\n0.0\n20\n-0.5\n30\n0.0\n11\n0.0\n21\n0.5\n31\n0.0\n");
    fprintf(dxf_file, "0\nENDBLK\n");
    fprintf(dxf_file, "0\nENDSEC\n");

    fprintf(dxf_file, "0\nSECTION\n2\nENTITIES\n");

    float current_x = xllcorner_value;
    float current_y = yllcorner_value + (nrows_value * cellsize_value);

    float scale_factor = 0.8 * (cellsize_value / 1.0);

    for (int row = 0; row < nrows_value; row++) {
        current_x = xllcorner_value;
        for (int col = 0; col < ncols_value; col++) {
            float z_value;
            if (fscanf(fp, "%f", &z_value) == 1) {
                if (z_value != nodata_float_value) {
                    if (((int)((current_x - xllcorner_value) / cellsize_value) % (int)(spacing_value / cellsize_value) == 0) && 
                        ((int)((current_y - yllcorner_value) / cellsize_value) % (int)(spacing_value / cellsize_value) == 0)) {
                        fprintf(dxf_file, "0\nINSERT\n8\n0\n2\nCrossBlock\n10\n%f\n20\n%f\n30\n%f\n", 
                                current_x, current_y, z_value);
                        
                        fprintf(dxf_file, "0\nTEXT\n8\n0\n10\n%f\n20\n%f\n30\n%f\n1\n%.2f\n40\n0.2\n", 
                                current_x + 0.25, current_y + 0.25, z_value, z_value);
                    }

                }
            } else {
                fprintf(stderr, "Error reading data at row %d, col %d\n", row, col);
                fclose(fp);
                fclose(dxf_file);
                return 1;
            }
            current_x += cellsize_value;
        }
        current_y -= cellsize_value;
    }

    fprintf(dxf_file, "0\nENDSEC\n");
    fprintf(dxf_file, "0\nEOF\n");
    fclose(fp);
    fclose(dxf_file);

    printf("Conversion completed successfully. Output saved to '%s'\n", output_file);

    return 0;
}
