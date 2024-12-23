#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.asc>\n", argv[0]);
        return 1;
    }

    char *input_file = argv[1];
    char output_file[256];

    strncpy(output_file, input_file, sizeof(output_file) - 5);
    output_file[sizeof(output_file) - 5] = '\0';
    char *dot = strrchr(output_file, '.');
    if (dot) *dot = '\0';
    strcat(output_file, ".csv");

    FILE *fp = fopen(input_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error opening input file '%s': %s\n", input_file, strerror(errno));
        return 1;
    }

    char line[255];
    int nrows_value = 0, ncols_value = 0, nodata_value = 0;
    float xllcorner_value = 0.0, yllcorner_value = 0.0, cellsize_value = 0.0;

    for (int i = 0; i < 6; i++) {
        fgets(line, sizeof(line), fp);
        if (strstr(line, "nrows")) sscanf(line, "%*s %d", &nrows_value);
        else if (strstr(line, "ncols")) sscanf(line, "%*s %d", &ncols_value);
        else if (strstr(line, "xllcorner")) sscanf(line, "%*s %f", &xllcorner_value);
        else if (strstr(line, "yllcorner")) sscanf(line, "%*s %f", &yllcorner_value);
        else if (strstr(line, "cellsize")) sscanf(line, "%*s %f", &cellsize_value);
        else if (strstr(line, "nodata_value")) sscanf(line, "%*s %d", &nodata_value);
    }

    printf("Header processed, generating '%s'\n", output_file);

    FILE *csv_file = fopen(output_file, "w");
    if (csv_file == NULL) {
        fprintf(stderr, "Error creating output file '%s': %s\n", output_file, strerror(errno));
        fclose(fp);
        return 1;
    }

    fprintf(csv_file, "X,Y,Z\n");

    float current_x = xllcorner_value;
    float current_y = yllcorner_value + (nrows_value * cellsize_value);

    for (int row = 0; row < nrows_value; row++) {
        current_x = xllcorner_value;
        for (int col = 0; col < ncols_value; col++) {
            float z_value;
            if (fscanf(fp, "%f", &z_value) == 1) {
                if ((int)z_value != nodata_value) {
                    fprintf(csv_file, "%f,%f,%f\n", current_x, current_y, z_value);
                }
            } else {
                fprintf(stderr, "Error reading data at row %d, col %d\n", row, col);
                fclose(fp);
                fclose(csv_file);
                return 1;
            }
            current_x += cellsize_value;
        }
        current_y -= cellsize_value;
    }

    printf("Conversion completed successfully. Output saved to '%s'\n", output_file);

    fclose(fp);
    fclose(csv_file);
    return 0;
}

