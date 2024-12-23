#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define NUM_PARTS 6
#define MAX_POINTS 100

typedef struct {
    float x;
    float y;
    float z;
    char code[10];
} Point;

void generate_output_filename(const char *input_filename, char *output_filename) {
    strcpy(output_filename, input_filename);

    char *dot_pos = strrchr(output_filename, '.');
    if (dot_pos != NULL) {
        strcpy(dot_pos, "_lines.geojson");
    } else {
        strcat(output_filename, "_lines.geojson");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_filename = argv[1];

    char output_filename[256];
    generate_output_filename(input_filename, output_filename);

    FILE *input_file = fopen(input_filename, "r");
    if (input_file == NULL) {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    FILE *output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        perror("Failed to open output file");
        fclose(input_file);
        return EXIT_FAILURE;
    }

    fprintf(output_file, "{\n");
    fprintf(output_file, "  \"type\": \"FeatureCollection\",\n");
    fprintf(output_file, "  \"features\": [\n");

    char line[MAX_LINE_LENGTH];
    Point line_points[MAX_POINTS];
    int point_count = 0;
    int is_first_feature = 1;

    while (fgets(line, sizeof(line), input_file)) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "21", 2) == 0) {
            char *parts[NUM_PARTS];
            char *token = strtok(line, ", ");
            int part_count = 0;

            while (token != NULL && part_count < NUM_PARTS) {
                parts[part_count++] = token;
                token = strtok(NULL, ", ");
            }

            if (part_count == NUM_PARTS) {
                if (strchr(parts[5], '.') != NULL) {
                    if (!is_first_feature) {
                        fprintf(output_file, "      ]\n");
                        fprintf(output_file, "    }\n");
                        fprintf(output_file, "  },\n");
                    }

                    point_count = 0;
                    is_first_feature = 0;

                    fprintf(output_file, "  {\n");
                    fprintf(output_file, "    \"type\": \"Feature\",\n");
                    fprintf(output_file, "    \"geometry\": {\n");
                    fprintf(output_file, "      \"type\": \"LineString\",\n");
                    fprintf(output_file, "      \"coordinates\": [\n");
                }

                if (point_count < MAX_POINTS) {
                    Point p;
                    p.x = atof(parts[2]);
                    p.y = atof(parts[3]);
                    p.z = atof(parts[4]);
                    strncpy(p.code, parts[5], sizeof(p.code) - 1);
                    p.code[sizeof(p.code) - 1] = '\0';
                    line_points[point_count++] = p;

                    if (point_count > 1) {
                        fprintf(output_file, ",\n");
                    }
                    fprintf(output_file, "        [%.3f, %.3f, %.3f]", p.x, p.y, p.z);
                }
            }
        }
    }

    if (point_count > 0) {
        fprintf(output_file, "\n      ]\n");
        fprintf(output_file, "    }\n");
        fprintf(output_file, "  }\n");
    }

    // Close the GeoJSON file
    fprintf(output_file, "  ]\n");
    fprintf(output_file, "}\n");

    fclose(input_file);
    fclose(output_file);

    printf("GeoJSON file created successfully: %s\n", output_filename);
    return EXIT_SUCCESS;
}