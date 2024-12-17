#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>

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
} LASPointFormat0;

#pragma pack(pop)

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
    strcat(output_file, ".las");

    LASHeader header = {0};
    LASPointFormat0 point;

    memcpy(header.file_signature, "LASF", 4);
    header.version_major = 1;
    header.version_minor = 2;
    strncpy(header.system_identifier, "SYSTEM_XYZ", 32);
    strncpy(header.generating_software, "ASCTOOLS GENERATOR", 32);
    header.file_creation_day = 300;
    header.file_creation_year = 2024;
    header.header_size = sizeof(LASHeader);
    header.point_data_format_id = 0;
    header.point_data_record_length = sizeof(LASPointFormat0);
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
    int nrows_value, ncols_value, nodata_value;
    float xllcorner_value, yllcorner_value, cellsize_value;

    for (int i = 0; i < 6; i++) {
        fgets(line, sizeof(line), fp);
        if (strstr(line, "nrows")) sscanf(line, "%*s %d", &nrows_value);
        else if (strstr(line, "ncols")) sscanf(line, "%*s %d", &ncols_value);
        else if (strstr(line, "xllcorner")) sscanf(line, "%*s %f", &xllcorner_value);
        else if (strstr(line, "yllcorner")) sscanf(line, "%*s %f", &yllcorner_value);
        else if (strstr(line, "cellsize")) sscanf(line, "%*s %f", &cellsize_value);
        else if (strstr(line, "nodata_value")) sscanf(line, "%*s %d", &nodata_value);
    }

    header.min_x = xllcorner_value;
    header.min_y = yllcorner_value;
    header.max_x = xllcorner_value + (ncols_value * cellsize_value);
    header.max_y = yllcorner_value + (nrows_value * cellsize_value);

    header.x_scale_factor = 0.01;
    header.y_scale_factor = 0.01;
    header.z_scale_factor = 0.01;

    double min_z = 9999999, max_z = -9999999;
    float current_x, current_y;
    current_y = yllcorner_value + nrows_value * cellsize_value;

    int point_counter = 0;
    for (int row = 0; row < nrows_value; row++) {
        current_x = xllcorner_value;
        for (int col = 0; col < ncols_value; col++) {
            float z_value;
            if (fscanf(fp, "%f", &z_value) != 1) {
                fprintf(stderr, "Error reading data at row %d, col %d\n", row, col);
                fclose(fp);
                fclose(las_file);
                return 1;
            }
            if ((int)z_value == nodata_value || (nodata_value != -9999 && (int)z_value == -9999)) {
                current_x += cellsize_value;
                continue;
            }

            if (z_value < min_z) min_z = z_value;
            if (z_value > max_z) max_z = z_value;

            point.x = (int32_t)(current_x / header.x_scale_factor);
            point.y = (int32_t)(current_y / header.y_scale_factor);
            point.z = (int32_t)(z_value / header.z_scale_factor);
            point.intensity = 100;
            point.return_number = 1;
            point.number_of_returns = 1;
            point.classification = 2;
            point.point_source_id = 1;

            fwrite(&point, sizeof(LASPointFormat0), 1, las_file);
            point_counter++;
            current_x += cellsize_value;
        }
        current_y -= cellsize_value;
    }

    header.num_point_records = point_counter;
    header.min_z = min_z;
    header.max_z = max_z;

    fseek(las_file, 0, SEEK_SET);
    fwrite(&header, sizeof(LASHeader), 1, las_file);

    printf("Conversion complete: '%s' -> '%s'. Total points: %d\n", input_file, output_file, point_counter);

    fclose(fp);
    fclose(las_file);
    return 0;
}

