#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define NUM_PARTS 6
#define MAX_POINTS 300
#define MAX_FEATURES 2500

typedef struct {
    float x;
    float y;
    float z;
} Vertex;

typedef struct {
    char code[10];
    Vertex *vertices;
    int vertex_count;
    int vertex_capacity;
} Feature;

void generate_output_filename(const char *input_filename, char *output_filename) {
    strcpy(output_filename, input_filename);

    char *dot_pos = strrchr(output_filename, '.');
    if (dot_pos != NULL) {
        strcpy(dot_pos, "_lines.dxf");
    } else {
        strcat(output_filename, "_lines.dxf");
    }
}

int feature_in_list(const char *feature_code, char **list, int list_count) {
    for (int i = 0; i < list_count; i++) {
        if (strcmp(feature_code, list[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file> [--one-code {x}] [--list-codes {x},{y},{z}]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_filename = argv[1];
    char *one_code = NULL;
    char *list_codes[MAX_FEATURES];
    int list_count = 0;

    // Parse additional arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--one-code") == 0 && i + 1 < argc) {
            one_code = argv[++i];
        } else if (strcmp(argv[i], "--list-codes") == 0 && i + 1 < argc) {
            char *token = strtok(argv[++i], ",");
            while (token != NULL && list_count < MAX_FEATURES) {
                list_codes[list_count++] = token;
                token = strtok(NULL, ",");
            }
        }
    }

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

    fprintf(output_file, "0\nSECTION\n");
    fprintf(output_file, "2\nHEADER\n");
    fprintf(output_file, "0\nENDSEC\n");

    fprintf(output_file, "0\nSECTION\n");
    fprintf(output_file, "2\nENTITIES\n");

    Feature features[MAX_FEATURES];
    int feature_count = 0;
    int current_feature_index = -1;

    char line[MAX_LINE_LENGTH];
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
                Vertex v;
                v.x = atof(parts[2]);
                v.y = atof(parts[3]);
                v.z = atof(parts[4]);

                char raw_code[10];
                strncpy(raw_code, parts[5], sizeof(raw_code) - 1);
                raw_code[sizeof(raw_code) - 1] = '\0';

                char cleaned_code[10] = "";
                char *src = raw_code, *dest = cleaned_code;
                while (*src) {
                    if (*src != '.') {
                        *dest++ = *src;
                    }
                    src++;
                }
                *dest = '\0';

                if (strchr(parts[5], '.') != NULL || current_feature_index == -1) {
                    current_feature_index = feature_count;
                    features[current_feature_index].vertex_capacity = MAX_POINTS;
                    features[current_feature_index].vertices = malloc(features[current_feature_index].vertex_capacity * sizeof(Vertex));
                    if (!features[current_feature_index].vertices) {
                        fprintf(stderr, "Memory allocation failed for feature %s.\n", cleaned_code);
                        fclose(input_file);
                        fclose(output_file);
                        return EXIT_FAILURE;
                    }
                    strncpy(features[current_feature_index].code, cleaned_code, sizeof(features[current_feature_index].code) - 1);
                    features[current_feature_index].code[sizeof(features[current_feature_index].code) - 1] = '\0';
                    features[current_feature_index].vertex_count = 0;
                    feature_count++;
                }

                Feature *current_feature = &features[current_feature_index];
                if (current_feature->vertex_count >= current_feature->vertex_capacity) {
                    current_feature->vertex_capacity *= 2;
                    current_feature->vertices = realloc(current_feature->vertices, current_feature->vertex_capacity * sizeof(Vertex));
                    if (!current_feature->vertices) {
                        fprintf(stderr, "Memory reallocation failed for vertices of feature %s.\n", cleaned_code);
                        fclose(input_file);
                        fclose(output_file);
                        return EXIT_FAILURE;
                    }
                }
                current_feature->vertices[current_feature->vertex_count++] = v;
            }
        }
    }

    for (int i = 0; i < feature_count; i++) {
        if (one_code && strcmp(features[i].code, one_code) != 0) {
            continue;
        }
        if (list_count > 0 && !feature_in_list(features[i].code, list_codes, list_count)) {
            continue;
        }

        fprintf(output_file, "0\nPOLYLINE\n");
        fprintf(output_file, "8\n%s\n", features[i].code);
        fprintf(output_file, "66\n1\n");
        fprintf(output_file, "70\n0\n");

        for (int j = 0; j < features[i].vertex_count; j++) {
            fprintf(output_file, "0\nVERTEX\n");
            fprintf(output_file, "8\n%s\n", features[i].code);
            fprintf(output_file, "10\n%.3f\n", features[i].vertices[j].x);
            fprintf(output_file, "20\n%.3f\n", features[i].vertices[j].y);
            fprintf(output_file, "30\n%.3f\n", features[i].vertices[j].z);
        }

        fprintf(output_file, "0\nSEQEND\n");
    }

    for (int i = 0; i < feature_count; i++) {
        free(features[i].vertices);
    }

    fprintf(output_file, "0\nENDSEC\n");
    fprintf(output_file, "0\nEOF\n");

    fclose(input_file);
    fclose(output_file);

    printf("DXF file created successfully: %s\n", output_filename);
    return EXIT_SUCCESS;
}
