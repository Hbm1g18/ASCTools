#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#pragma pack(push, 1)

typedef struct {
    char file_signature[4];
    uint16_t file_source_id;
    uint16_t global_encoding;
    uint32_t project_id_1;
    uint16_t project_id_2;
    uint16_t project_id_3;
    uint8_t project_id_4[8];
    uint8_t version_major;
    uint8_t version_minor;
    char system_identifier[32];
    char generating_software[32];
    uint16_t file_creation_day;
    uint16_t file_creation_year;
    uint16_t header_size;
    uint32_t offset_to_point_data;
    uint32_t num_variable_length_recs;
    uint8_t point_data_format_id;
    uint16_t point_data_record_length;
    uint32_t num_point_records;
    uint32_t num_points_by_return[5];
    double x_scale_factor;
    double y_scale_factor;
    double z_scale_factor;
    double x_offset;
    double y_offset;
    double z_offset;
    double max_x;
    double min_x;
    double max_y;
    double min_y;
    double max_z;
    double min_z;
} LASHeader;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
    uint16_t intensity;
    uint8_t return_number : 3;
    uint8_t number_of_returns : 3;
    uint8_t scan_direction_flag : 1;
    uint8_t edge_of_flight_line : 1;
    uint8_t classification;
    int8_t scan_angle_rank;
    uint8_t user_data;
    uint16_t point_source_id;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} LASPointFormat2;

#pragma pack(pop)

void viridis_colormap(double normalized, uint16_t *red, uint16_t *green, uint16_t *blue) {
    double r = 0.0, g = 0.0, b = 0.0;
    if (normalized < 0.0) normalized = 0.0;
    if (normalized > 1.0) normalized = 1.0;

    // Colour ramp using viridis if -elev_rgb is used to try and add a bit of colour to clouds
    if (normalized <= 0.25) {
        r = 0.267 + normalized * 4.0 * (0.282 - 0.267);
        g = 0.004 + normalized * 4.0 * (0.141 - 0.004);
        b = 0.329 + normalized * 4.0 * (0.435 - 0.329);
    } else if (normalized <= 0.5) {
        normalized = (normalized - 0.25) * 4.0;
        r = 0.282 + normalized * (0.127 - 0.282);
        g = 0.141 + normalized * (0.570 - 0.141);
        b = 0.435 + normalized * (0.704 - 0.435);
    } else if (normalized <= 0.75) {
        normalized = (normalized - 0.5) * 4.0;
        r = 0.127 + normalized * (0.267 - 0.127);
        g = 0.570 + normalized * (0.678 - 0.570);
        b = 0.704 + normalized * (0.653 - 0.704);
    } else {
        normalized = (normalized - 0.75) * 4.0;
        r = 0.267 + normalized * (0.993 - 0.267);
        g = 0.678 + normalized * (0.906 - 0.678);
        b = 0.653 + normalized * (0.569 - 0.653);
    }

    *red = (uint16_t)(r * 65535.0);
    *green = (uint16_t)(g * 65535.0);
    *blue = (uint16_t)(b * 65535.0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.00{x}> [-elev_rgb]\n", argv[0]);
        return 1;
    }

    int use_elevation_color = 0;
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "-elev_rgb") == 0) {
            use_elevation_color = 1;
        }
    }

    char *input_file = argv[1];
    char output_file[256];
    strncpy(output_file, input_file, sizeof(output_file) - 5);
    output_file[sizeof(output_file) - 5] = '\0';
    char *dot = strrchr(output_file, '.');
    if (dot) *dot = '\0';
    strcat(output_file, ".las");

    LASHeader header = {0};
    LASPointFormat2 point;

    memcpy(header.file_signature, "LASF", 4);
    header.version_major = 1;
    header.version_minor = 2;
    strncpy(header.system_identifier, "SYSTEM_XYZ", 32);
    strncpy(header.generating_software, "LSS2LAS GENERATOR", 32);
    header.file_creation_day = 300;
    header.file_creation_year = 2024;
    header.header_size = sizeof(LASHeader);
    header.point_data_format_id = 2;
    header.point_data_record_length = sizeof(LASPointFormat2);
    header.offset_to_point_data = sizeof(LASHeader);

    FILE *fp = fopen(input_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error opening input file '%s': %s\n", input_file, strerror(errno));
        return 1;
    }

    FILE *las_file = fopen(output_file, "wb");
    if (las_file == NULL) {
        fprintf(stderr, "Error creating output file '%s': %s\n", output_file, strerror(errno));
        fclose(fp);
        return 1;
    }

    fwrite(&header, sizeof(LASHeader), 1, las_file);

    char line[255];
    double min_x = 9999999, max_x = -9999999;
    double min_y = 9999999, max_y = -9999999;
    double min_z = 9999999, max_z = -9999999;
    int point_counter = 0;

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

        if (field_count < 5) continue;

        double x = atof(fields[2]);
        double y = atof(fields[3]);
        double z = atof(fields[4]);

        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        if (z < min_z) min_z = z;
        if (z > max_z) max_z = z;

        point.x = (int32_t)(x / 0.01);
        point.y = (int32_t)(y / 0.01);
        point.z = (int32_t)(z / 0.01);
        point.intensity = 100;
        point.return_number = 1;
        point.number_of_returns = 1;
        point.classification = 2;
        point.point_source_id = 1;

        if (use_elevation_color) {
            double normalized = (z - min_z) / (max_z - min_z);
            viridis_colormap(normalized, &point.red, &point.green, &point.blue);
        } else {
            point.red = point.green = point.blue = 0;
        }

        fwrite(&point, sizeof(LASPointFormat2), 1, las_file);
        point_counter++;
    }

    header.num_point_records = point_counter;
    header.min_x = min_x;
    header.max_x = max_x;
    header.min_y = min_y;
    header.max_y = max_y;
    header.min_z = min_z;
    header.max_z = max_z;
    header.x_scale_factor = 0.01;
    header.y_scale_factor = 0.01;
    header.z_scale_factor = 0.01;

    fseek(las_file, 0, SEEK_SET);
    fwrite(&header, sizeof(LASHeader), 1, las_file);

    printf("Conversion complete. Output file: %s. Total points: %d\n", output_file, point_counter);

    fclose(fp);
    fclose(las_file);

    return 0;
}
