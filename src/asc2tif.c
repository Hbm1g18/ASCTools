#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

void write_geotiff(const char *filename, int ncols, int nrows, float xllcorner, float yllcorner, float cellsize, float *data, int epsg_code) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Cannot open GeoTIFF file");
        return;
    }

    // Step 1: Write TIFF header
    uint16_t byte_order = 0x4949;
    uint16_t version = 42;
    uint32_t ifd_offset = 8;

    fwrite(&byte_order, sizeof(byte_order), 1, file);
    fwrite(&version, sizeof(version), 1, file);
    fwrite(&ifd_offset, sizeof(ifd_offset), 1, file);

    uint16_t num_entries = 11;
    fwrite(&num_entries, sizeof(num_entries), 1, file);

    struct TiffTag {
        uint16_t tag_id;
        uint16_t data_type;
        uint32_t count;
        uint32_t value_offset;
    } tag;

    // ImageWidth
    tag.tag_id = 256;
    tag.data_type = 4;
    tag.count = 1;
    tag.value_offset = ncols;
    fwrite(&tag, sizeof(tag), 1, file);

    // ImageLength
    tag.tag_id = 257;
    tag.data_type = 4;
    tag.count = 1;
    tag.value_offset = nrows;
    fwrite(&tag, sizeof(tag), 1, file);

    // BitsPerSample
    tag.tag_id = 258;
    tag.data_type = 3;
    tag.count = 1;
    tag.value_offset = 32;
    fwrite(&tag, sizeof(tag), 1, file);

    // SampleFormat (Floating Point)
    tag.tag_id = 339;
    tag.data_type = 3;
    tag.count = 1;
    tag.value_offset = 3;
    fwrite(&tag, sizeof(tag), 1, file);

    // Compression
    tag.tag_id = 259;
    tag.data_type = 3;
    tag.count = 1;
    tag.value_offset = 1;
    fwrite(&tag, sizeof(tag), 1, file);

    // PhotometricInterpretation
    tag.tag_id = 262;
    tag.data_type = 3;
    tag.count = 1;
    tag.value_offset = 1;
    fwrite(&tag, sizeof(tag), 1, file);

    // StripOffsets
    uint32_t strip_offset = ftell(file) + 2 + num_entries * sizeof(tag) + 4;
    tag.tag_id = 273;
    tag.data_type = 4;
    tag.count = 1;
    tag.value_offset = strip_offset;
    fwrite(&tag, sizeof(tag), 1, file);

    // RowsPerStrip
    tag.tag_id = 278;
    tag.data_type = 4;
    tag.count = 1;
    tag.value_offset = nrows;
    fwrite(&tag, sizeof(tag), 1, file);

    // StripByteCounts
    uint32_t strip_byte_count = ncols * nrows * sizeof(float);
    tag.tag_id = 279;
    tag.data_type = 4;
    tag.count = 1;
    tag.value_offset = strip_byte_count;
    fwrite(&tag, sizeof(tag), 1, file);

    // ModelPixelScaleTag
    uint32_t model_pixel_scale_offset = strip_offset + strip_byte_count;
    tag.tag_id = 33550;
    tag.data_type = 12;
    tag.count = 3;
    tag.value_offset = model_pixel_scale_offset;
    fwrite(&tag, sizeof(tag), 1, file);

    // ModelTiepointTag
    uint32_t model_tiepoint_offset = model_pixel_scale_offset + 3 * sizeof(double);
    tag.tag_id = 33922;
    tag.data_type = 12;
    tag.count = 6;
    tag.value_offset = model_tiepoint_offset;
    fwrite(&tag, sizeof(tag), 1, file);

    // GeoKeyDirectoryTag
    uint32_t geo_key_dir_offset = model_tiepoint_offset + 6 * sizeof(double);
    tag.tag_id = 34735;
    tag.data_type = 3;
    tag.count = 4;
    tag.value_offset = geo_key_dir_offset;
    fwrite(&tag, sizeof(tag), 1, file);

    // Step 3: Write raster data
    fseek(file, strip_offset, SEEK_SET);
    for (int i = 0; i < ncols * nrows; i++) {
        float value = data[i];
        fwrite(&value, sizeof(float), 1, file);
    }

    // Step 4: Write geospatial metadata
    double pixel_scale[3] = {cellsize, cellsize, 0.0};
    fwrite(pixel_scale, sizeof(double), 3, file);

    double tiepoint[6] = {0.0, 0.0, 0.0, xllcorner, yllcorner + (nrows * cellsize), 0.0};
    fwrite(tiepoint, sizeof(double), 6, file);

    uint16_t geo_key_dir[4] = {1, 1, 0, (uint16_t)epsg_code};
    fwrite(geo_key_dir, sizeof(uint16_t), 4, file);

    fclose(file);
    printf("GeoTIFF file created: %s\n", filename);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.asc> <epsg_code>\n", argv[0]);
        return 1;
    }

    char *input_file = argv[1];
    int epsg_code = atoi(argv[2]);

    char output_file[256];
    strncpy(output_file, input_file, sizeof(output_file) - 5);
    output_file[sizeof(output_file) - 5] = '\0';
    char *dot = strrchr(output_file, '.');
    if (dot) *dot = '\0';
    strcat(output_file, ".tif");

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

    float *data = malloc(nrows_value * ncols_value * sizeof(float));
    if (!data) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(fp);
        return 1;
    }

    for (int row = 0; row < nrows_value; row++) {
        for (int col = 0; col < ncols_value; col++) {
            float z_value;
            if (fscanf(fp, "%f", &z_value) == 1) {
                data[row * ncols_value + col] = z_value;
            } else {
                fprintf(stderr, "Error reading data at row %d, col %d\n", row, col);
                free(data);
                fclose(fp);
                return 1;
            }
        }
    }

    fclose(fp);

    write_geotiff(output_file, ncols_value, nrows_value, xllcorner_value, yllcorner_value, cellsize_value, data, epsg_code);

    free(data);
    return 0;
}
