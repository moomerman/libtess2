#+build !js
package libtess2

import "core:c"
import "core:testing"

// -- Basic API tests ----------------------------------------------------------

@(test)
test_new_delete :: proc(t: ^testing.T) {
	tess := NewTess(nil)
	testing.expect(t, tess != nil, "NewTess should return a valid handle")
	DeleteTess(tess)
}

@(test)
test_tessellate_triangle :: proc(t: ^testing.T) {
	tess := NewTess(nil)
	defer DeleteTess(tess)

	verts := [3][2]f32{{0, 0}, {100, 0}, {50, 100}}
	AddContour(tess, 2, raw_data(&verts), size_of([2]f32), 3)

	normal := [3]f32{0, 0, 1}
	result := Tesselate(tess, c.int(Winding_Rule.Odd), c.int(Element_Type.Polygons), 3, 2, raw_data(&normal))
	testing.expect(t, result != 0, "tessellation should succeed")

	vert_count := int(GetVertexCount(tess))
	elem_count := int(GetElementCount(tess))
	testing.expect(t, vert_count == 3, "triangle should have 3 vertices")
	testing.expect(t, elem_count == 1, "triangle should produce 1 element")
}

@(test)
test_tessellate_quad :: proc(t: ^testing.T) {
	tess := NewTess(nil)
	defer DeleteTess(tess)

	verts := [4][2]f32{{0, 0}, {100, 0}, {100, 100}, {0, 100}}
	AddContour(tess, 2, raw_data(&verts), size_of([2]f32), 4)

	normal := [3]f32{0, 0, 1}
	result := Tesselate(tess, c.int(Winding_Rule.Odd), c.int(Element_Type.Polygons), 3, 2, raw_data(&normal))
	testing.expect(t, result != 0, "tessellation should succeed")

	vert_count := int(GetVertexCount(tess))
	elem_count := int(GetElementCount(tess))
	testing.expect(t, vert_count >= 4, "quad should have at least 4 vertices")
	testing.expect(t, elem_count >= 2, "quad should produce at least 2 triangles")
}

@(test)
test_connected_polygons :: proc(t: ^testing.T) {
	// Test the Connected_Polygons element type which provides adjacency info.
	tess := NewTess(nil)
	defer DeleteTess(tess)

	verts := [4][2]f32{{0, 0}, {100, 0}, {100, 100}, {0, 100}}
	AddContour(tess, 2, raw_data(&verts), size_of([2]f32), 4)

	normal := [3]f32{0, 0, 1}
	result := Tesselate(tess, c.int(Winding_Rule.Odd), c.int(Element_Type.Connected_Polygons), 3, 2, raw_data(&normal))
	testing.expect(t, result != 0, "tessellation should succeed")

	elem_count := int(GetElementCount(tess))
	elems := GetElements(tess)
	testing.expect(t, elems != nil, "elements pointer should be valid")

	// With Connected_Polygons and poly_size=3, each element has 6 ints:
	// [v0, v1, v2, n0, n1, n2] where n values are neighbor indices or UNDEF.
	has_neighbor := false
	for i in 0 ..< elem_count {
		for e in 0 ..< 3 {
			nb := elems[i * 6 + 3 + e]
			if nb != UNDEF {
				has_neighbor = true
			}
		}
	}
	testing.expect(t, elem_count >= 2 && has_neighbor, "connected polygons should have adjacency data")
}

@(test)
test_constrained_delaunay :: proc(t: ^testing.T) {
	tess := NewTess(nil)
	defer DeleteTess(tess)

	SetOption(tess, c.int(Option.Constrained_Delaunay_Triangulation), 1)

	verts := [4][2]f32{{0, 0}, {200, 0}, {200, 100}, {0, 100}}
	AddContour(tess, 2, raw_data(&verts), size_of([2]f32), 4)

	normal := [3]f32{0, 0, 1}
	result := Tesselate(tess, c.int(Winding_Rule.Odd), c.int(Element_Type.Polygons), 3, 2, raw_data(&normal))
	testing.expect(t, result != 0, "CDT tessellation should succeed")
	testing.expect(t, int(GetElementCount(tess)) >= 2, "should produce triangles")
}

@(test)
test_polygon_with_hole :: proc(t: ^testing.T) {
	tess := NewTess(nil)
	defer DeleteTess(tess)

	outer := [4][2]f32{{0, 0}, {200, 0}, {200, 200}, {0, 200}}
	hole := [4][2]f32{{50, 50}, {150, 50}, {150, 150}, {50, 150}}

	AddContour(tess, 2, raw_data(&outer), size_of([2]f32), 4)
	AddContour(tess, 2, raw_data(&hole), size_of([2]f32), 4)

	normal := [3]f32{0, 0, 1}
	result := Tesselate(tess, c.int(Winding_Rule.Odd), c.int(Element_Type.Polygons), 3, 2, raw_data(&normal))
	testing.expect(t, result != 0, "tessellation with hole should succeed")

	elem_count := int(GetElementCount(tess))
	testing.expect(t, elem_count >= 4, "polygon with hole should produce multiple triangles")
}

// -- C interop tests ----------------------------------------------------------

@(test)
test_vertex_data_roundtrip :: proc(t: ^testing.T) {
	// Verify that vertex data passes through the C boundary correctly.
	tess := NewTess(nil)
	defer DeleteTess(tess)

	input := [3][2]f32{{1.5, 2.5}, {100.25, 0.75}, {50.125, 99.875}}
	AddContour(tess, 2, raw_data(&input), size_of([2]f32), 3)

	normal := [3]f32{0, 0, 1}
	result := Tesselate(tess, c.int(Winding_Rule.Odd), c.int(Element_Type.Polygons), 3, 2, raw_data(&normal))
	testing.expect(t, result != 0, "tessellation should succeed")

	vert_count := int(GetVertexCount(tess))
	verts := GetVertices(tess)
	testing.expect(t, vert_count == 3, "should have 3 vertices")

	// All input vertices should appear in the output (order may differ).
	for iv in input {
		found := false
		for i in 0 ..< vert_count {
			ox := verts[i * 2]
			oy := verts[i * 2 + 1]
			if abs(ox - iv.x) < 1e-6 && abs(oy - iv.y) < 1e-6 {
				found = true
				break
			}
		}
		testing.expectf(t, found, "input vertex (%.3f, %.3f) should appear in output", iv.x, iv.y)
	}
}

@(test)
test_element_indices_valid :: proc(t: ^testing.T) {
	// Verify element indices are within the vertex count range.
	tess := NewTess(nil)
	defer DeleteTess(tess)

	verts := [5][2]f32{{0, 0}, {100, 0}, {120, 80}, {60, 120}, {-20, 80}}
	AddContour(tess, 2, raw_data(&verts), size_of([2]f32), 5)

	normal := [3]f32{0, 0, 1}
	result := Tesselate(tess, c.int(Winding_Rule.Odd), c.int(Element_Type.Polygons), 3, 2, raw_data(&normal))
	testing.expect(t, result != 0, "tessellation should succeed")

	vert_count := GetVertexCount(tess)
	elem_count := int(GetElementCount(tess))
	elems := GetElements(tess)

	for i in 0 ..< elem_count {
		for j in 0 ..< 3 {
			idx := elems[i * 3 + j]
			testing.expectf(
				t,
				idx >= 0 && idx < vert_count,
				"element[%d][%d] = %d should be in range [0, %d)",
				i, j, idx, vert_count,
			)
		}
	}
}

@(test)
test_vertex_indices :: proc(t: ^testing.T) {
	// Test GetVertexIndices — maps output vertices back to input order.
	tess := NewTess(nil)
	defer DeleteTess(tess)

	verts := [4][2]f32{{0, 0}, {100, 0}, {100, 100}, {0, 100}}
	AddContour(tess, 2, raw_data(&verts), size_of([2]f32), 4)

	normal := [3]f32{0, 0, 1}
	Tesselate(tess, c.int(Winding_Rule.Odd), c.int(Element_Type.Polygons), 3, 2, raw_data(&normal))

	vert_count := int(GetVertexCount(tess))
	indices := GetVertexIndices(tess)
	testing.expect(t, indices != nil, "vertex indices should be valid")

	for i in 0 ..< vert_count {
		idx := indices[i]
		valid := idx == UNDEF || (idx >= 0 && idx < c.int(4))
		testing.expectf(t, valid, "vertex index[%d] = %d should be UNDEF or in input range", i, idx)
	}
}

@(test)
test_winding_rules :: proc(t: ^testing.T) {
	// Verify all winding rules produce valid output on overlapping contours.
	rules := [?]Winding_Rule{.Odd, .Nonzero, .Positive, .Negative, .Abs_Geq_Two}

	for rule in rules {
		tess := NewTess(nil)
		defer DeleteTess(tess)

		// Two overlapping squares.
		sq1 := [4][2]f32{{0, 0}, {100, 0}, {100, 100}, {0, 100}}
		sq2 := [4][2]f32{{50, 50}, {150, 50}, {150, 150}, {50, 150}}
		AddContour(tess, 2, raw_data(&sq1), size_of([2]f32), 4)
		AddContour(tess, 2, raw_data(&sq2), size_of([2]f32), 4)

		normal := [3]f32{0, 0, 1}
		result := Tesselate(tess, c.int(rule), c.int(Element_Type.Polygons), 3, 2, raw_data(&normal))
		testing.expectf(t, result != 0, "winding rule %v should succeed", rule)
	}
}
