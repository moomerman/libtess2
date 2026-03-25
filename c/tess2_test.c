// C tests for the pre-built libtess2 libraries.
//
// These validate that the compiled .a/.lib links and works correctly from
// plain C — the baseline sanity check before any language bindings.
//
// Build & run:  sh run.sh   (from this directory)

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// -- libtess2 declarations (no vendored header needed) ------------------------

typedef struct TESStesselator TESStesselator;

enum TessWindingRule {
    TESS_WINDING_ODD,
    TESS_WINDING_NONZERO,
    TESS_WINDING_POSITIVE,
    TESS_WINDING_NEGATIVE,
    TESS_WINDING_ABS_GEQ_TWO,
};

enum TessElementType {
    TESS_POLYGONS,
    TESS_CONNECTED_POLYGONS,
    TESS_BOUNDARY_CONTOURS,
};

enum TessOption {
    TESS_CONSTRAINED_DELAUNAY_TRIANGULATION,
    TESS_REVERSE_CONTOURS,
};

#define TESS_UNDEF (~0)

TESStesselator* tessNewTess(void* alloc);
void            tessDeleteTess(TESStesselator* tess);
void            tessAddContour(TESStesselator* tess, int size, const void* pointer, int stride, int count);
void            tessSetOption(TESStesselator* tess, int option, int value);
int             tessTesselate(TESStesselator* tess, int windingRule, int elementType, int polySize, int vertexSize, const float* normal);
int             tessGetVertexCount(TESStesselator* tess);
const float*    tessGetVertices(TESStesselator* tess);
const int*      tessGetVertexIndices(TESStesselator* tess);
int             tessGetElementCount(TESStesselator* tess);
const int*      tessGetElements(TESStesselator* tess);

// -- Test harness -------------------------------------------------------------

static int tests_run    = 0;
static int tests_passed = 0;

#define ASSERT(cond, msg) do {                                     \
    if (!(cond)) {                                                 \
        printf("  FAIL: %s (line %d)\n", msg, __LINE__);           \
        return 0;                                                  \
    }                                                              \
} while (0)

#define RUN(fn) do {                                               \
    tests_run++;                                                   \
    printf("  %s ... ", #fn);                                      \
    if (fn()) { tests_passed++; printf("ok\n"); }                  \
    else      { printf("\n"); }                                    \
} while (0)

// -- Tests --------------------------------------------------------------------

static int test_new_delete(void) {
    TESStesselator* tess = tessNewTess(NULL);
    ASSERT(tess != NULL, "tessNewTess should return a valid handle");
    tessDeleteTess(tess);
    return 1;
}

static int test_triangle(void) {
    TESStesselator* tess = tessNewTess(NULL);
    ASSERT(tess != NULL, "tessNewTess");

    float verts[] = { 0,0, 100,0, 50,100 };
    tessAddContour(tess, 2, verts, sizeof(float) * 2, 3);

    float normal[] = { 0, 0, 1 };
    int ok = tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, normal);
    ASSERT(ok, "tessTesselate should succeed");
    ASSERT(tessGetVertexCount(tess) == 3, "should have 3 vertices");
    ASSERT(tessGetElementCount(tess) == 1, "should have 1 triangle");

    tessDeleteTess(tess);
    return 1;
}

static int test_quad(void) {
    TESStesselator* tess = tessNewTess(NULL);

    float verts[] = { 0,0, 100,0, 100,100, 0,100 };
    tessAddContour(tess, 2, verts, sizeof(float) * 2, 4);

    float normal[] = { 0, 0, 1 };
    int ok = tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, normal);
    ASSERT(ok, "tessTesselate should succeed");
    ASSERT(tessGetVertexCount(tess) >= 4, "should have at least 4 vertices");
    ASSERT(tessGetElementCount(tess) >= 2, "should have at least 2 triangles");

    tessDeleteTess(tess);
    return 1;
}

static int test_polygon_with_hole(void) {
    TESStesselator* tess = tessNewTess(NULL);

    float outer[] = { 0,0, 200,0, 200,200, 0,200 };
    float hole[]  = { 50,50, 150,50, 150,150, 50,150 };
    tessAddContour(tess, 2, outer, sizeof(float) * 2, 4);
    tessAddContour(tess, 2, hole, sizeof(float) * 2, 4);

    float normal[] = { 0, 0, 1 };
    int ok = tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, normal);
    ASSERT(ok, "tessTesselate with hole should succeed");
    ASSERT(tessGetElementCount(tess) >= 4, "should produce multiple triangles");

    tessDeleteTess(tess);
    return 1;
}

static int test_connected_polygons(void) {
    TESStesselator* tess = tessNewTess(NULL);

    float verts[] = { 0,0, 100,0, 100,100, 0,100 };
    tessAddContour(tess, 2, verts, sizeof(float) * 2, 4);

    float normal[] = { 0, 0, 1 };
    int ok = tessTesselate(tess, TESS_WINDING_ODD, TESS_CONNECTED_POLYGONS, 3, 2, normal);
    ASSERT(ok, "tessTesselate should succeed");

    int elem_count = tessGetElementCount(tess);
    const int* elems = tessGetElements(tess);
    ASSERT(elems != NULL, "elements should be valid");

    // Each element is 6 ints: [v0, v1, v2, n0, n1, n2].
    int has_neighbor = 0;
    for (int i = 0; i < elem_count; i++) {
        for (int e = 0; e < 3; e++) {
            if (elems[i * 6 + 3 + e] != (int)TESS_UNDEF)
                has_neighbor = 1;
        }
    }
    ASSERT(elem_count >= 2 && has_neighbor, "should have adjacency data");

    tessDeleteTess(tess);
    return 1;
}

static int test_constrained_delaunay(void) {
    TESStesselator* tess = tessNewTess(NULL);

    tessSetOption(tess, TESS_CONSTRAINED_DELAUNAY_TRIANGULATION, 1);

    float verts[] = { 0,0, 200,0, 200,100, 0,100 };
    tessAddContour(tess, 2, verts, sizeof(float) * 2, 4);

    float normal[] = { 0, 0, 1 };
    int ok = tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, normal);
    ASSERT(ok, "CDT tessTesselate should succeed");
    ASSERT(tessGetElementCount(tess) >= 2, "should produce triangles");

    tessDeleteTess(tess);
    return 1;
}

static int test_vertex_data_roundtrip(void) {
    TESStesselator* tess = tessNewTess(NULL);

    float input[] = { 1.5f, 2.5f, 100.25f, 0.75f, 50.125f, 99.875f };
    tessAddContour(tess, 2, input, sizeof(float) * 2, 3);

    float normal[] = { 0, 0, 1 };
    int ok = tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, normal);
    ASSERT(ok, "tessTesselate should succeed");

    int vert_count = tessGetVertexCount(tess);
    const float* verts = tessGetVertices(tess);
    ASSERT(vert_count == 3, "should have 3 vertices");

    // Every input vertex should appear in the output.
    for (int iv = 0; iv < 3; iv++) {
        float ix = input[iv * 2], iy = input[iv * 2 + 1];
        int found = 0;
        for (int ov = 0; ov < vert_count; ov++) {
            if (fabsf(verts[ov * 2] - ix) < 1e-6f &&
                fabsf(verts[ov * 2 + 1] - iy) < 1e-6f) {
                found = 1;
                break;
            }
        }
        ASSERT(found, "input vertex should appear in output");
    }

    tessDeleteTess(tess);
    return 1;
}

static int test_element_indices_valid(void) {
    TESStesselator* tess = tessNewTess(NULL);

    float verts[] = { 0,0, 100,0, 120,80, 60,120, -20,80 };
    tessAddContour(tess, 2, verts, sizeof(float) * 2, 5);

    float normal[] = { 0, 0, 1 };
    int ok = tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, normal);
    ASSERT(ok, "tessTesselate should succeed");

    int vert_count = tessGetVertexCount(tess);
    int elem_count = tessGetElementCount(tess);
    const int* elems = tessGetElements(tess);

    for (int i = 0; i < elem_count; i++) {
        for (int j = 0; j < 3; j++) {
            int idx = elems[i * 3 + j];
            ASSERT(idx >= 0 && idx < vert_count, "element index out of range");
        }
    }

    tessDeleteTess(tess);
    return 1;
}

static int test_vertex_indices(void) {
    TESStesselator* tess = tessNewTess(NULL);

    float verts[] = { 0,0, 100,0, 100,100, 0,100 };
    tessAddContour(tess, 2, verts, sizeof(float) * 2, 4);

    float normal[] = { 0, 0, 1 };
    tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, normal);

    int vert_count = tessGetVertexCount(tess);
    const int* indices = tessGetVertexIndices(tess);
    ASSERT(indices != NULL, "vertex indices should be valid");

    for (int i = 0; i < vert_count; i++) {
        int idx = indices[i];
        ASSERT(idx == (int)TESS_UNDEF || (idx >= 0 && idx < 4),
               "vertex index should be TESS_UNDEF or in input range");
    }

    tessDeleteTess(tess);
    return 1;
}

static int test_winding_rules(void) {
    int rules[] = {
        TESS_WINDING_ODD, TESS_WINDING_NONZERO, TESS_WINDING_POSITIVE,
        TESS_WINDING_NEGATIVE, TESS_WINDING_ABS_GEQ_TWO,
    };

    for (int r = 0; r < 5; r++) {
        TESStesselator* tess = tessNewTess(NULL);

        float sq1[] = { 0,0, 100,0, 100,100, 0,100 };
        float sq2[] = { 50,50, 150,50, 150,150, 50,150 };
        tessAddContour(tess, 2, sq1, sizeof(float) * 2, 4);
        tessAddContour(tess, 2, sq2, sizeof(float) * 2, 4);

        float normal[] = { 0, 0, 1 };
        int ok = tessTesselate(tess, rules[r], TESS_POLYGONS, 3, 2, normal);
        ASSERT(ok, "winding rule should succeed");

        tessDeleteTess(tess);
    }
    return 1;
}

// -- Main ---------------------------------------------------------------------

int main(void) {
    printf("libtess2 C tests:\n");

    RUN(test_new_delete);
    RUN(test_triangle);
    RUN(test_quad);
    RUN(test_polygon_with_hole);
    RUN(test_connected_polygons);
    RUN(test_constrained_delaunay);
    RUN(test_vertex_data_roundtrip);
    RUN(test_element_indices_valid);
    RUN(test_vertex_indices);
    RUN(test_winding_rules);

    printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
