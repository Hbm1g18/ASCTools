#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

typedef struct {
    double x, y;
} Point;

int compare(const void *a, const void *b) {
    Point *p1 = (Point *)a;
    Point *p2 = (Point *)b;
    if (p1->x != p2->x)
        return (p1->x > p2->x) - (p1->x < p2->x);
    return (p1->y > p2->y) - (p1->y < p2->y);
}

double cross(Point o, Point a, Point b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

void compute_convex_hull(Point *points, int n, Point **hull, int *hull_size) {
    *hull = malloc(n * sizeof(Point));
    if (!*hull) {
        fprintf(stderr, "Memory allocation failed for hull.\n");
        exit(1);
    }

    qsort(points, n, sizeof(Point), compare);

    int k = 0;
    for (int i = 0; i < n; ++i) {
        while (k >= 2 && cross((*hull)[k - 2], (*hull)[k - 1], points[i]) <= 0)
            k--;
        (*hull)[k++] = points[i];
    }

    for (int i = n - 2, t = k + 1; i >= 0; --i) {
        while (k >= t && cross((*hull)[k - 2], (*hull)[k - 1], points[i]) <= 0)
            k--;
        (*hull)[k++] = points[i];
    }
    *hull_size = k - 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.00{x}>\n", argv[0]);
        return 1;
    }

    char *input_file = argv[1];
    char output_file[256];

    strncpy(output_file, input_file, sizeof(output_file) - 10);
    output_file[sizeof(output_file) - 10] = '\0';
    char *dot = strrchr(output_file, '.');
    if (dot) *dot = '\0';
    strcat(output_file, "_boundary.geojson");

    FILE *fp = fopen(input_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error opening input file '%s': %s\n", input_file, strerror(errno));
        return 1;
    }

    int capacity = 1000;
    int point_count = 0;
    Point *points = malloc(capacity * sizeof(Point));
    if (!points) {
        fprintf(stderr, "Memory allocation failed for points.\n");
        fclose(fp);
        return 1;
    }

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

        if (point_count >= capacity) {
            capacity *= 2;
            points = realloc(points, capacity * sizeof(Point));
            if (!points) {
                fprintf(stderr, "Memory reallocation failed for points.\n");
                fclose(fp);
                return 1;
            }
        }

        points[point_count].x = atof(fields[2]);
        points[point_count].y = atof(fields[3]);
        point_count++;
    }
    fclose(fp);

    if (point_count < 3) {
        fprintf(stderr, "Not enough points to form a convex hull.\n");
        free(points);
        return 1;
    }

    Point *hull;
    int hull_size;
    compute_convex_hull(points, point_count, &hull, &hull_size);
    free(points);

    FILE *out_fp = fopen(output_file, "w");
    if (out_fp == NULL) {
        fprintf(stderr, "Error creating output file '%s': %s\n", output_file, strerror(errno));
        free(hull);
        return 1;
    }

    fprintf(out_fp, "{\n  \"type\": \"FeatureCollection\",\n  \"features\": [\n    {\n");
    fprintf(out_fp, "      \"type\": \"Feature\",\n      \"geometry\": {\n        \"type\": \"Polygon\",\n        \"coordinates\": [\n          [\n");

    for (int i = 0; i < hull_size; ++i) {
        fprintf(out_fp, "            [%.6f, %.6f]%s\n", hull[i].x, hull[i].y, (i == hull_size - 1) ? "" : ",");
    }
    fprintf(out_fp, "          ]\n        ]\n      },\n      \"properties\": {}\n    }\n  ]\n}\n");

    fclose(out_fp);
    free(hull);

    printf("Boundary GeoJSON output complete. Output file: %s\n", output_file);
    return 0;
}
