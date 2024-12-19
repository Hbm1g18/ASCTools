#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

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

    double min_x = 9999999, max_x = -9999999;
    double min_y = 9999999, max_y = -9999999;
    double min_z = 9999999, max_z = -9999999;
    double total_z = 0.0;
    int link_count = 0;

    char line[255];
    char feature_codes[1000][10];
    int feature_code_count = 0;

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

        char *fields[6];
        int field_count = 0;
        char *token = strtok(clean_line, ",");
        while (token != NULL && field_count < 6) {
            fields[field_count++] = token;
            token = strtok(NULL, ",");
        }

        if (field_count < 5) {
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

        double x = atof(fields[2]);
        double y = atof(fields[3]);
        double z = atof(fields[4]);

        points[point_count].x = x;
        points[point_count].y = y;
        point_count++;

        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        if (z < min_z) min_z = z;
        if (z > max_z) max_z = z;

        total_z += z;

        if (field_count == 6 && strchr(fields[5], '.') != NULL) {
            link_count++;
        }

        if (field_count == 6) {
            char feature_code[10];
            char *src_code = fields[5];
            char *dest_code = feature_code;
            while (*src_code) {
                if (*src_code != '.') {
                    *dest_code++ = *src_code;
                }
                src_code++;
            }
            *dest_code = '\0';

            int found = 0;
            for (int i = 0; i < feature_code_count; i++) {
                if (strcmp(feature_codes[i], feature_code) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                strncpy(feature_codes[feature_code_count], feature_code, 9);
                feature_codes[feature_code_count][9] = '\0';
                feature_code_count++;
            }
        }
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

    double centroid_x = 0.0, centroid_y = 0.0;
    for (int i = 0; i < hull_size; ++i) {
        centroid_x += hull[i].x;
        centroid_y += hull[i].y;
    }
    centroid_x /= hull_size;
    centroid_y /= hull_size;

    printf("Total points in survey: %d\n", point_count);
    printf("Total links in survey: %d\n", link_count);
    printf("Min_x, Min_y, Min_z: %.6f, %.6f, %.6f\n", min_x, min_y, min_z);
    printf("Max_x, Max_y, Max_z: %.6f, %.6f, %.6f\n", max_x, max_y, max_z);
    printf("Average Z: %.6f\n", total_z / point_count);
    printf("Boundary: ");
    for (int i = 0; i < hull_size; ++i) {
        printf("[%.6f,%.6f]%s", hull[i].x, hull[i].y, (i == hull_size - 1) ? "" : ",");
    }
    printf("\n");
    printf("Centroid: [%.6f,%.6f]\n", centroid_x, centroid_y);

    printf("Feature codes present: ");
    for (int i = 0; i < feature_code_count; i++) {
        printf("%s%s", feature_codes[i], (i == feature_code_count - 1) ? "" : ",");
    }
    printf("\n");

    free(hull);

    return 0;
}