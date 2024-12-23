#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.00{x}>\n", argv[0]);
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

    FILE *out_fp = fopen(output_file, "w");
    if (out_fp == NULL) {
        fprintf(stderr, "Error creating output file '%s': %s\n", output_file, strerror(errno));
        fclose(fp);
        return 1;
    }

    fprintf(out_fp, "x,y,z\n");

    char line[255];
    while (fgets(line, sizeof(line), fp)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';

        if (strncmp(line, "21", 2) != 0) continue;

        char clean_line[255] = "";
        char *src = line, *dest = clean_line;
        while (*src) {
            if (!isspace((unsigned char)*src)) {
                *dest++ = *src;
            }
            src++;
        }
        *dest = '\0';

        char *fields[5];
        int field_count = 0;
        char *token = strtok(clean_line, ",");
        while (token != NULL && field_count < 5) {
            fields[field_count++] = token;
            token = strtok(NULL, ",");
        }

        if (field_count < 4) {
            fprintf(stderr, "Malformed line: %s\n", line);
            continue;
        }

        char *x = fields[2];
        char *y = fields[3];
        char *z = (field_count > 4) ? fields[4] : "";

        fprintf(out_fp, "%s,%s,%s\n", x, y, z);
    }

    fclose(fp);
    fclose(out_fp);

    printf("Conversion complete. Output file: %s\n", output_file);
    return 0;
}
