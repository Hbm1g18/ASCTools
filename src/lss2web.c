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
        strcpy(dot_pos, "_map.html");
    } else {
        strcat(output_filename, "_map.html");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file> [-ge]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_filename = argv[1];
    int include_google_earth = 0;
    int include_points = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-ge") == 0) {
            include_google_earth = 1;
        } else if (strcmp(argv[i], "-points") == 0) {
            include_points = 1;
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

    fprintf(output_file, "<html>\n");
    fprintf(output_file, "<head>\n");
    fprintf(output_file, "  <title>LSS2WEB CONVERSION</title>\n");
    fprintf(output_file, "  <meta charset=\"utf-8\" />\n");
    fprintf(output_file, "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(output_file, "  <link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.css\" />\n");
    fprintf(output_file, "  <script src=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.js\"></script>\n");
    fprintf(output_file, "  <script src=\"https://cdnjs.cloudflare.com/ajax/libs/proj4js/2.15.0/proj4.js\"></script>\n");
    fprintf(output_file, "  <style>#map { height: 99vh; }</style>\n");
    fprintf(output_file, "</head>\n");
    fprintf(output_file, "<body>\n");
    fprintf(output_file, "  <div id=\"map\"></div>\n");
    fprintf(output_file, "  <script>\n");
    fprintf(output_file, "      proj4.defs([\n");
    fprintf(output_file, "        [\n");
    fprintf(output_file, "            'EPSG:27700',\n");
    fprintf(output_file, "            '+proj=tmerc +lat_0=49 +lon_0=-2 +k=0.9996012717 +x_0=400000 +y_0=-100000 +ellps=airy +towgs84=446.448,-125.157,542.06,0.15,0.247,0.842,-20.489 +units=m +no_defs +type=crs'\n");
    fprintf(output_file, "        ]\n");
    fprintf(output_file, "      ]);\n");

    fprintf(output_file, "      function transformCoordinatesToLatLng(coords) {\n");
    fprintf(output_file, "        return coords.map(coord => {\n");
    fprintf(output_file, "          const transformed = proj4('EPSG:27700', 'EPSG:4326', [coord[0], coord[1]]);\n");
    fprintf(output_file, "          return [transformed[1], transformed[0]];\n");
    fprintf(output_file, "        });\n");
    fprintf(output_file, "      }\n");
    fprintf(output_file, "      const map = L.map('map').setView([52.5074, -0.08], 7);\n");
    fprintf(output_file, "      const osmLayer = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {\n");
    fprintf(output_file, "        attribution: '&copy; <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors',\n");
    fprintf(output_file, "      });\n");
    fprintf(output_file, "      osmLayer.addTo(map);\n");

    if (include_google_earth) {
        fprintf(output_file, "      const googleEarthLayer = L.tileLayer('https://{s}.google.com/vt/lyrs=s&x={x}&y={y}&z={z}', {\n");
        fprintf(output_file, "        attribution: '&copy; <a href=\"https://www.google.com/earth/\">Google Earth</a>',\n");
        fprintf(output_file, "        subdomains: ['mt0', 'mt1', 'mt2', 'mt3'],\n");
        fprintf(output_file, "        maxZoom: 20,\n");
        fprintf(output_file, "      });\n");
    }

    fprintf(output_file, "      const linesLayer = L.layerGroup();\n");
    if (include_points){
        fprintf(output_file, "      const pointsLayer = L.layerGroup();\n");
    }

    fprintf(output_file, "      const baseLayers = {\n");
    fprintf(output_file, "        'OpenStreetMap': osmLayer,\n");

    if (include_google_earth) {
        fprintf(output_file, "        'Google Earth': googleEarthLayer,\n");
    }

    fprintf(output_file, "      };\n");

    fprintf(output_file, "      const overlayLayers = {\n");
    fprintf(output_file, "        'Lines': linesLayer,\n");
    if (include_points){
        fprintf(output_file, "        'Points': pointsLayer,\n");
    }
    fprintf(output_file, "      };\n");

    fprintf(output_file, "      L.control.layers(baseLayers, overlayLayers).addTo(map);\n");

    char line[MAX_LINE_LENGTH];
    Point line_points[MAX_POINTS];
    int point_count = 0;
    int is_first_feature = 1;
    int polyline_counter = 0;

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
                float x = atof(parts[2]);
                float y = atof(parts[3]);
                float z = atof(parts[4]);
                char *code = parts[5];

                if (strchr(code, '.') != NULL) {
                    if (!is_first_feature) {
                        fprintf(output_file, "      ];\n");
                    }
                    polyline_counter++;
                    fprintf(output_file, "      const line%d = [\n", polyline_counter);
                    is_first_feature = 0;
                    point_count = 0;
                }
                fprintf(output_file, "        [%f, %f],\n", x, y);
                point_count++;
            }
        }
    }

    if (point_count > 0) {
        fprintf(output_file, "      ];\n");
    }

    for (int i = 1; i <= polyline_counter; i++) {
        fprintf(output_file, "      const transformedLine%d = transformCoordinatesToLatLng(line%d);\n", i, i);
        fprintf(output_file, "      const polyline%d = L.polyline(transformedLine%d, { weight: 2 });\n", i, i);
        fprintf(output_file, "      linesLayer.addLayer(polyline%d);\n", i);
    }

    fprintf(output_file, "      linesLayer.addTo(map);\n");

    if (include_points){
        char line[MAX_LINE_LENGTH];
        float min_z = 999999, max_z = -999999;

        rewind(input_file);

        fprintf(output_file, "const points = [\n");
        while (fgets(line, sizeof(line), input_file)) {
            char *parts[NUM_PARTS];
            int part_count = 0;
            char *token = strtok(line, ", ");
            while (token != NULL && part_count < NUM_PARTS) {
                parts[part_count++] = token;
                token = strtok(NULL, ", ");
            }

            if (part_count == NUM_PARTS) {
                float x = atof(parts[2]);
                float y = atof(parts[3]);
                float z = atof(parts[4]);
                if (x == 0.0 ) {
                    continue; 
                }
                if (z < min_z) min_z = z;
                if (z > max_z) max_z = z;
                fprintf(output_file, "  {lat: %f, lng: %f, z: %f},\n", y, x, z);
            }
        }
        fprintf(output_file, "];\n");

        fprintf(output_file, "const minZ = Math.min(...points.map(p => p.z));\n");
        fprintf(output_file, "const maxZ = Math.max(...points.map(p => p.z));\n");
        fprintf(output_file, "function getColor(z) {\n");
        fprintf(output_file, "    const normalized = (z - minZ) / (maxZ - minZ);\n");
        fprintf(output_file, "    const hue = (1 - normalized) * 240;\n");
        fprintf(output_file, "    return `hsl(${hue}, 100%%, 50%%)`;\n");
        fprintf(output_file, "}\n");

        fprintf(output_file, "points.forEach(p => {\n");
        fprintf(output_file, "    const transformed = proj4('EPSG:27700', 'EPSG:4326', [p.lng, p.lat]);\n");
        fprintf(output_file, "    p.lat = transformed[1];\n    p.lng = transformed[0];\n");
        fprintf(output_file, "    const marker = L.circleMarker([p.lat, p.lng], { radius: 1, color: getColor(p.z) });\n");
        fprintf(output_file, "    marker.bindPopup(`Elevation: ${p.z} m`).openPopup();\n");
        fprintf(output_file, "    pointsLayer.addLayer(marker);\n");
        fprintf(output_file, "});\n");

        fprintf(output_file, "      pointsLayer.addTo(map);\n");
    }

    fprintf(output_file, "    </script>\n");
    fprintf(output_file, "</body>\n");
    fprintf(output_file, "</html>\n");

    fclose(input_file);
    fclose(output_file);

    printf("HTML file created successfully: %s\n", output_filename);
    return EXIT_SUCCESS;
}
