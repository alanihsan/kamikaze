/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "object_nodes.h"

#include <kamikaze/mesh.h>
#include <kamikaze/noise.h>
#include <kamikaze/primitive.h>
#include <kamikaze/prim_points.h>
#include <kamikaze/util_parallel.h>
#include <kamikaze/utils_glm.h>

#include <random>
#include <sstream>

#include "ui/paramfactory.h"

/* ************************************************************************** */

OutputNode::OutputNode(const std::string &name)
    : Node(name)
{
	addInput("Primitive");
}

PrimitiveCollection *OutputNode::collection() const
{
	return m_collection;
}

void OutputNode::process()
{
	m_collection = getInputCollection("Primitive");
}

/* ************************************************************************** */

TransformNode::TransformNode()
    : Node("Transform")
{
	addInput("Prim");
	addOutput("Prim");

	EnumProperty xform_enum_prop;
	xform_enum_prop.insert("Pre Transform", 0);
	xform_enum_prop.insert("Post Transform", 1);

	add_prop("xform_order", "Transform Order", property_type::prop_enum);
	set_prop_enum_values(xform_enum_prop);

	EnumProperty rot_enum_prop;
	rot_enum_prop.insert("X Y Z", 0);
	rot_enum_prop.insert("X Z Y", 1);
	rot_enum_prop.insert("Y X Z", 2);
	rot_enum_prop.insert("Y Z X", 3);
	rot_enum_prop.insert("Z X Y", 4);
	rot_enum_prop.insert("Z Y X", 5);

	add_prop("rot_order", "Rotation Order", property_type::prop_enum);
	set_prop_enum_values(rot_enum_prop);

	add_prop("translate", "Translate", property_type::prop_vec3);
	set_prop_min_max(-10.0f, 10.0f);
	set_prop_default_value_vec3(glm::vec3{0.0f, 0.0f, 0.0f});

	add_prop("rotate", "Rotate", property_type::prop_vec3);
	set_prop_min_max(0.0f, 360.0f);
	set_prop_default_value_vec3(glm::vec3{0.0f, 0.0f, 0.0f});

	add_prop("scale", "Scale", property_type::prop_vec3);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_vec3(glm::vec3{1.0f, 1.0f, 1.0f});

	add_prop("pivot", "Pivot", property_type::prop_vec3);
	set_prop_min_max(-10.0f, 10.0f);
	set_prop_default_value_vec3(glm::vec3{0.0f, 0.0f, 0.0f});

	add_prop("uniform_scale", "Uniform Scale", property_type::prop_float);
	set_prop_min_max(0.0f, 1000.0f);
	set_prop_default_value_float(1.0f);

	add_prop("invert_xform", "Invert Transformation", property_type::prop_bool);
}

void TransformNode::process()
{
	const auto translate = eval_vec3("translate");
	const auto rotate = eval_vec3("rotate");
	const auto scale = eval_vec3("scale");
	const auto pivot = eval_vec3("pivot");
	const auto uniform_scale = eval_float("uniform_scale");
	const auto transform_type = eval_int("xform_order");
	const auto rot_order = eval_int("rot_order");

	/* determine the rotatation order */
	int rot_ord[6][3] = {
	    { 0, 1, 2 }, // X Y Z
	    { 0, 2, 1 }, // X Z Y
	    { 1, 0, 2 }, // Y X Z
	    { 1, 2, 0 }, // Y Z X
	    { 2, 0, 1 }, // Z X Y
	    { 2, 1, 0 }, // Z Y X
	};

	glm::vec3 axis[3] = {
	    glm::vec3(1.0f, 0.0f, 0.0f),
	    glm::vec3(0.0f, 1.0f, 0.0f),
	    glm::vec3(0.0f, 0.0f, 1.0f),
	};

	const auto X = rot_ord[rot_order][0];
	const auto Y = rot_ord[rot_order][1];
	const auto Z = rot_ord[rot_order][2];

	for (auto &prim : primitive_iterator(this->m_collection)) {
		auto matrix = glm::mat4(1.0f);

		switch (transform_type) {
			case 0: /* Pre Transform */
				matrix = pre_translate(matrix, pivot);
				matrix = pre_rotate(matrix, glm::radians(rotate[X]), axis[X]);
				matrix = pre_rotate(matrix, glm::radians(rotate[Y]), axis[Y]);
				matrix = pre_rotate(matrix, glm::radians(rotate[Z]), axis[Z]);
				matrix = pre_scale(matrix, scale * uniform_scale);
				matrix = pre_translate(matrix, -pivot);
				matrix = pre_translate(matrix, translate);
				matrix = matrix * prim->matrix();
				break;
			case 1: /* Post Transform */
				matrix = post_translate(matrix, pivot);
				matrix = post_rotate(matrix, glm::radians(rotate[X]), axis[X]);
				matrix = post_rotate(matrix, glm::radians(rotate[Y]), axis[Y]);
				matrix = post_rotate(matrix, glm::radians(rotate[Z]), axis[Z]);
				matrix = post_scale(matrix, scale * uniform_scale);
				matrix = post_translate(matrix, -pivot);
				matrix = post_translate(matrix, translate);
				matrix = prim->matrix() * matrix;
				break;
		}

		prim->matrix(matrix);
	}
}

/* ************************************************************************** */

CreateBoxNode::CreateBoxNode()
    : Node("Box")
{
	addOutput("Prim");

	add_prop("size", "Size", property_type::prop_vec3);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_vec3(glm::vec3{1.0f, 1.0f, 1.0f});

	add_prop("center", "Center", property_type::prop_vec3);
	set_prop_min_max(-10.0f, 10.0f);
	set_prop_default_value_vec3(glm::vec3{0.0f, 0.0f, 0.0f});

	add_prop("uniform_scale", "Uniform Scale", property_type::prop_float);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_float(1.0f);
}

void CreateBoxNode::process()
{
	auto prim = m_collection->build("Mesh");
	auto mesh = static_cast<Mesh *>(prim);

	PointList *points = mesh->points();
	points->reserve(8);

	const auto dimension = eval_vec3("size");
	const auto center = eval_vec3("center");
	const auto uniform_scale = eval_float("uniform_scale");

	/* todo: expose this to the UI */
	const auto &x_div = 2;
	const auto &y_div = 2;
	const auto &z_div = 2;

	auto vec = glm::vec3{ 0.0f, 0.0f, 0.0f };

	const auto size = dimension * uniform_scale;

	const auto &start_x = -(size.x / 2.0f) + center.x;
	const auto &start_y = -(size.y / 2.0f) + center.y;
	const auto &start_z = -(size.z / 2.0f) + center.z;

	const auto &x_increment = size.x / (x_div - 1);
	const auto &y_increment = size.y / (y_div - 1);
	const auto &z_increment = size.z / (z_div - 1);

	for (auto x = 0; x < x_div; ++x) {
		vec[0] = start_x + x * x_increment;

		for (auto y = 0; y < y_div; ++y) {
			vec[1] = start_y + y * y_increment;

			for (auto z = 0; z < z_div; ++z) {
				vec[2] = start_z + z * z_increment;

				points->push_back(vec);
			}
		}
	}

	PolygonList *polys = mesh->polys();
	polys->resize(6);
	polys->push_back(glm::uvec4(1, 3, 2, 0));
	polys->push_back(glm::uvec4(3, 7, 6, 2));
	polys->push_back(glm::uvec4(7, 5, 4, 6));
	polys->push_back(glm::uvec4(5, 1, 0, 4));
	polys->push_back(glm::uvec4(0, 2, 6, 4));
	polys->push_back(glm::uvec4(5, 7, 3, 1));

	mesh->tagUpdate();
}

/* ************************************************************************** */

CreateTorusNode::CreateTorusNode()
    : Node("Torus")
{
	addOutput("Prim");

	add_prop("center", "Center", property_type::prop_vec3);
	set_prop_min_max(-10.0f, 10.0f);
	set_prop_default_value_vec3(glm::vec3{0.0f, 0.0f, 0.0f});

	add_prop("major_radius", "Major Radius", property_type::prop_float);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_float(1.0f);

	add_prop("minor_radius", "Minor Radius", property_type::prop_float);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_float(0.25f);

	add_prop("major_segment", "Major Segment", property_type::prop_int);
	set_prop_min_max(4, 100);
	set_prop_default_value_int(48);

	add_prop("minor_segment", "Minor Segment", property_type::prop_int);
	set_prop_min_max(4, 100);
	set_prop_default_value_int(24);

	add_prop("uniform_scale", "Uniform Scale", property_type::prop_float);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_float(1.0f);
}

void CreateTorusNode::process()
{
	auto prim = m_collection->build("Mesh");
	auto mesh = static_cast<Mesh *>(prim);

	const auto center = eval_vec3("center");
	const auto uniform_scale = eval_float("uniform_scale");

	const auto major_radius = eval_float("major_radius") * uniform_scale;
	const auto minor_radius = eval_float("minor_radius") * uniform_scale;
	const auto major_segment = eval_int("major_segment");
	const auto minor_segment = eval_int("minor_segment");

	PointList *points = mesh->points();
	PolygonList *polys = mesh->polys();

	constexpr auto tau = static_cast<float>(M_PI) * 2.0f;

	const auto vertical_angle_stride = tau / static_cast<float>(major_segment);
	const auto horizontal_angle_stride = tau / static_cast<float>(minor_segment);

	int f1 = 0, f2, f3, f4;
	const auto tot_verts = major_segment * minor_segment;

	points->reserve(tot_verts);

	for (int i = 0; i < major_segment; ++i) {
		auto theta = vertical_angle_stride * i;

		for (int j = 0; j < minor_segment; ++j) {
			auto phi = horizontal_angle_stride * j;

			auto x = glm::cos(theta) * (major_radius + minor_radius * glm::cos(phi));
			auto y = minor_radius * glm::sin(phi);
			auto z = glm::sin(theta) * (major_radius + minor_radius * glm::cos(phi));

			points->push_back(glm::vec3(x, y, z) + center);

			if (j + 1 == minor_segment) {
				f2 = i * minor_segment;
				f3 = f1 + minor_segment;
				f4 = f2 + minor_segment;
			}
			else {
				f2 = f1 + 1;
				f3 = f1 + minor_segment;
				f4 = f3 + 1;
			}

			if (f2 >= tot_verts) {
				f2 -= tot_verts;
			}
			if (f3 >= tot_verts) {
				f3 -= tot_verts;
			}
			if (f4 >= tot_verts) {
				f4 -= tot_verts;
			}

			if (f2 > 0) {
				polys->push_back(glm::uvec4(f1, f3, f4, f2));
			}
			else {
				polys->push_back(glm::uvec4(f2, f1, f3, f4));
			}

			++f1;
		}
	}

	mesh->tagUpdate();
}

/* ************************************************************************** */

CreateGridNode::CreateGridNode()
    : Node("Grid")
{
	addOutput("Prim");

	add_prop("center", "Center", property_type::prop_vec3);
	set_prop_min_max(-10.0f, 10.0f);
	set_prop_default_value_vec3(glm::vec3{0.0f, 0.0f, 0.0f});

	add_prop("size", "Size", property_type::prop_vec3);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_vec3(glm::vec3{1.0f, 1.0f, 1.0f});

	add_prop("rows", "Rows", property_type::prop_int);
	set_prop_min_max(2, 100);
	set_prop_default_value_int(2);

	add_prop("columns", "Columns", property_type::prop_int);
	set_prop_min_max(2, 100);
	set_prop_default_value_int(2);
}

void CreateGridNode::process()
{
	auto prim = m_collection->build("Mesh");
	auto mesh = static_cast<Mesh *>(prim);

	const auto size = eval_vec3("size");
	const auto center = eval_vec3("center");

	const auto rows = eval_int("rows");
	const auto columns = eval_int("columns");

	const auto totpoints = rows * columns;

	auto points = mesh->points();
	points->reserve(totpoints);

	auto vec = glm::vec3{ 0.0f, center.y, 0.0f };

	const auto &x_increment = size.x / (rows - 1);
	const auto &y_increment = size.y / (columns - 1);
	const auto &start_x = -(size.x / 2.0f) + center.x;
	const auto &start_y = -(size.y / 2.0f) + center.z;

	for (auto y = 0; y < columns; ++y) {
		vec[2] = start_y + y * y_increment;

		for (auto x = 0; x < rows; ++x) {
			vec[0] = start_x + x * x_increment;

			points->push_back(vec);
		}
	}

	PolygonList *polys = mesh->polys();

	auto quad = glm::uvec4{ 0, 0, 0, 0 };

	/* make a copy for the lambda */
	const auto xtot = rows;

	auto index = [&xtot](const int x, const int y)
	{
		return x + y * xtot;
	};

	for (auto y = 1; y < columns; ++y) {
		for (auto x = 1; x < rows; ++x) {
			quad[0] = index(x - 1, y - 1);
			quad[1] = index(x,     y - 1);
			quad[2] = index(x,     y    );
			quad[3] = index(x - 1, y    );

			polys->push_back(quad);
		}
	}

	mesh->tagUpdate();
}

/* ************************************************************************** */

class CreateCircleNode : public Node {
public:
	CreateCircleNode();

	void process() override;
};

CreateCircleNode::CreateCircleNode()
    : Node("Circle")
{
	addOutput("Primitive");

	add_prop("vertices", "Vertices", property_type::prop_int);
	set_prop_min_max(3, 500);
	set_prop_default_value_int(32);

	add_prop("radius", "Radius", property_type::prop_float);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_float(1.0f);
}

void CreateCircleNode::process()
{
	auto prim = m_collection->build("Mesh");
	auto mesh = static_cast<Mesh *>(prim);

	const auto segs = eval_int("vertices");
	const auto dia = eval_float("radius");

	const auto phid = 2.0f * static_cast<float>(M_PI) / segs;
	auto phi = 0.0f;

	PointList *points = mesh->points();
	points->reserve(segs + 1);

	glm::vec3 vec(0.0f, 0.0f, 0.0f);

	points->push_back(vec);

	for (int a = 0; a < segs; ++a, phi += phid) {
		/* Going this way ends up with normal(s) upward */
		vec[0] = -dia * std::sin(phi);
		vec[2] = dia * std::cos(phi);

		points->push_back(vec);
	}

	PolygonList *polys = mesh->polys();

	auto index = points->size() - 1;
	glm::uvec4 poly(0, 0, 0, INVALID_INDEX);

	for (auto i = 1ul; i < points->size(); ++i) {
		poly[1] = index;
		poly[2] = i;

		polys->push_back(poly);

		index = i;
	}

	mesh->tagUpdate();
}

/* ************************************************************************** */

static void create_cylinder(PointList *points, PolygonList *polys, int segs, float dia1, float dia2, float depth)
{
	const auto phid = 2.0f * static_cast<float>(M_PI) / segs;
	auto phi = 0.0f;

	points->reserve((dia2 != 0.0f) ? segs * 2 + 2 : segs + 2);

	glm::vec3 vec(0.0f, 0.0f, 0.0f);

	const auto cent1 = 0;
	vec[1] = -depth;
	points->push_back(vec);

	const auto cent2 = 1;
	vec[1] = depth;
	points->push_back(vec);

	auto firstv1 = 0;
	auto firstv2 = 0;
	auto lastv1 = 0;
	auto lastv2 = 0;
	auto v1 = 0;
	auto v2 = 0;

	for (int a = 0; a < segs; ++a, phi += phid) {
		/* Going this way ends up with normal(s) upward */
		vec[0] = -dia1 * std::sin(phi);
		vec[1] = -depth;
		vec[2] = dia1 * std::cos(phi);

		v1 = points->size();
		points->push_back(vec);

		vec[0] = -dia2 * std::sin(phi);
		vec[1] = depth;
		vec[2] = dia2 * std::cos(phi);

		v2 = points->size();
		points->push_back(vec);

		if (a > 0) {
			/* Poly for the bottom cap. */
			polys->push_back(glm::uvec4{ cent1, lastv1, v1, INVALID_INDEX });

			/* Poly for the top cap. */
			polys->push_back(glm::uvec4{ cent2, v2, lastv2, INVALID_INDEX });

			/* Poly for the side. */
			polys->push_back(glm::uvec4{ lastv1, lastv2, v2, v1 });
		}
		else {
			firstv1 = v1;
			firstv2 = v2;
		}

		lastv1 = v1;
		lastv2 = v2;
	}

	/* Poly for the bottom cap. */
	polys->push_back(glm::uvec4{ cent1, v1, firstv1, INVALID_INDEX });

	/* Poly for the top cap. */
	polys->push_back(glm::uvec4{ cent2, firstv2, v2, INVALID_INDEX });

	/* Poly for the side. */
	polys->push_back(glm::uvec4{ v1, v2, firstv2, firstv1 });
}

class CreateTubeNode : public Node {
public:
	CreateTubeNode();

	void process() override;
};

CreateTubeNode::CreateTubeNode()
    : Node("Tube")
{
	addOutput("Primitive");

	add_prop("vertices", "Vertices", property_type::prop_int);
	set_prop_min_max(3, 500);
	set_prop_default_value_int(32);

	add_prop("radius", "Radius", property_type::prop_float);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_float(1.0f);

	add_prop("depth", "Depth", property_type::prop_float);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_float(1.0f);
}

void CreateTubeNode::process()
{
	auto prim = m_collection->build("Mesh");
	auto mesh = static_cast<Mesh *>(prim);

	const auto segs = eval_int("vertices");
	const auto dia = eval_float("radius");
	const auto depth = eval_float("depth");

	create_cylinder(mesh->points(), mesh->polys(), segs, dia, dia, depth);

	mesh->tagUpdate();
}

/* ************************************************************************** */

class CreateConeNode : public Node {
public:
	CreateConeNode();

	void process() override;
};

CreateConeNode::CreateConeNode()
    : Node("Cone")
{
	addOutput("Primitive");

	add_prop("vertices", "Vertices", property_type::prop_int);
	set_prop_min_max(3, 500);
	set_prop_default_value_int(32);

	add_prop("minor_radius", "Minor Radius", property_type::prop_float);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_float(0.0f);

	add_prop("major_radius", "Major Radius", property_type::prop_float);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_float(1.0f);

	add_prop("depth", "Depth", property_type::prop_float);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_float(1.0f);
}

void CreateConeNode::process()
{
	auto prim = m_collection->build("Mesh");
	auto mesh = static_cast<Mesh *>(prim);

	const auto segs = eval_int("vertices");
	const auto dia1 = eval_float("major_radius");
	const auto dia2 = eval_float("minor_radius");
	const auto depth = eval_float("depth");

	create_cylinder(mesh->points(), mesh->polys(), segs, dia1, dia2, depth);

	mesh->tagUpdate();
}

/* ************************************************************************** */

static const float icovert[12][3] = {
	{0.0f, 0.0f, -200.0f},
	{144.72f, -105.144f, -89.443f},
	{-55.277f, -170.128, -89.443f},
	{-178.885f, 0.0f, -89.443f},
	{-55.277f, 170.128f, -89.443f},
	{144.72f, 105.144f, -89.443f},
	{55.277f, -170.128f, 89.443f},
	{-144.72f, -105.144f, 89.443f},
	{-144.72f, 105.144f, 89.443f},
	{55.277f, 170.128f, 89.443f},
	{178.885f, 0.0f, 89.443f},
	{0.0f, 0.0f, 200.0f}
};

static const short icoface[20][3] = {
	{0, 1, 2},
	{1, 0, 5},
	{0, 2, 3},
	{0, 3, 4},
	{0, 4, 5},
	{1, 5, 10},
	{2, 1, 6},
	{3, 2, 7},
	{4, 3, 8},
	{5, 4, 9},
	{1, 10, 6},
	{2, 6, 7},
	{3, 7, 8},
	{4, 8, 9},
	{5, 9, 10},
	{6, 10, 11},
	{7, 6, 11},
	{8, 7, 11},
	{9, 8, 11},
	{10, 9, 11}
};

class CreateIcoSphereNode : public Node {
public:
	CreateIcoSphereNode();

	void process() override;
};

CreateIcoSphereNode::CreateIcoSphereNode()
    : Node("IcoSphere")
{
	addOutput("Primitive");

	add_prop("radius", "Radius", property_type::prop_float);
	set_prop_min_max(0.0f, 10.0f);
	set_prop_default_value_float(1.0f);
}

void CreateIcoSphereNode::process()
{
	auto prim = m_collection->build("Mesh");
	auto mesh = static_cast<Mesh *>(prim);

	const auto dia = eval_float("radius");
	const auto dia_div = dia / 200.0f;

	PointList *points = mesh->points();
	points->reserve(12);

	glm::vec3 vec(0.0f, 0.0f, 0.0f);

	for (int a = 0; a < 12; a++) {
		vec[0] = dia_div * icovert[a][0];
		vec[1] = dia_div * icovert[a][2];
		vec[2] = dia_div * icovert[a][1];

		points->push_back(vec);
	}

	PolygonList *polys = mesh->polys();
	polys->reserve(20);

	glm::uvec4 poly(0, 0, 0, INVALID_INDEX);

	for (auto i = 0; i < 20; ++i) {
		poly[0] = icoface[i][0];
		poly[1] = icoface[i][1];
		poly[2] = icoface[i][2];

		polys->push_back(poly);
	}

	mesh->tagUpdate();
}

/* ************************************************************************** */

static inline glm::vec3 get_normal(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2)
{
	const auto n0 = v0 - v1;
	const auto n1 = v2 - v1;

	return glm::cross(n1, n0);
}

class NormalNode : public Node {
public:
	NormalNode()
	    : Node("Normal")
	{
		addInput("input");
		addOutput("output");

		add_prop("flip", "Flip", property_type::prop_bool);
	}

	void process() override
	{
		const auto flip = eval_bool("flip");

		for (auto &prim : primitive_iterator(this->m_collection, Mesh::id)) {
			auto mesh = static_cast<Mesh *>(prim);
			auto normals = mesh->attribute("normal", ATTR_TYPE_VEC3);
			auto points = mesh->points();

			normals->resize(points->size());

			auto polys = mesh->polys();

			for (size_t i = 0, ie = points->size(); i < ie ; ++i) {
				normals->vec3(i, glm::vec3(0.0f));
			}

			parallel_for(tbb::blocked_range<size_t>(0, polys->size()),
			             [&](const tbb::blocked_range<size_t> &r)
			{
				for (auto i = r.begin(), ie = r.end(); i < ie ; ++i) {
					const auto &quad = (*polys)[i];

					const auto v0 = (*points)[quad[0]];
					const auto v1 = (*points)[quad[1]];
					const auto v2 = (*points)[quad[2]];

					const auto normal = get_normal(v0, v1, v2);

					normals->vec3(quad[0], normals->vec3(quad[0]) + normal);
					normals->vec3(quad[1], normals->vec3(quad[1]) + normal);
					normals->vec3(quad[2], normals->vec3(quad[2]) + normal);

					if (quad[3] != INVALID_INDEX) {
						normals->vec3(quad[3], normals->vec3(quad[3]) + normal);
					}
				}
			});

			if (flip) {
				for (size_t i = 0, ie = points->size(); i < ie ; ++i) {
					normals->vec3(i, -glm::normalize(normals->vec3(i)));
				}
			}
		}
	}
};

/* ************************************************************************** */

class NoiseNode : public Node {
public:
	NoiseNode()
	    : Node("Noise")
	{
		addInput("input");
		addOutput("output");

		add_prop("octaves", "Octaves", property_type::prop_int);
		set_prop_min_max(1, 10);
		set_prop_default_value_int(1);

		add_prop("frequency", "Frequency", property_type::prop_float);
		set_prop_min_max(0.0f, 1.0f);
		set_prop_default_value_float(1.0f);

		add_prop("amplitude", "Amplitude", property_type::prop_float);
		set_prop_min_max(0.0f, 10.0f);
		set_prop_default_value_float(1.0f);

		add_prop("persistence", "Persistence", property_type::prop_float);
		set_prop_min_max(0.0f, 10.0f);
		set_prop_default_value_float(1.0f);

		add_prop("lacunarity", "Lacunarity", property_type::prop_float);
		set_prop_min_max(0.0f, 10.0f);
		set_prop_default_value_float(2.0f);
	}

	void process() override
	{
		const auto octaves = eval_int("octaves");
		const auto lacunarity = eval_float("lacunarity");
		const auto persistence = eval_float("persistence");
		const auto ofrequency = eval_float("frequency");
		const auto oamplitude = eval_float("amplitude");

		for (auto prim : primitive_iterator(this->m_collection)) {
			PointList *points;

			if (prim->typeID() == Mesh::id) {
				auto mesh = static_cast<Mesh *>(prim);
				points = mesh->points();
			}
			else if (prim->typeID() == PrimPoints::id) {
				auto prim_points = static_cast<PrimPoints *>(prim);
				points = prim_points->points();
			}
			else {
				continue;
			}

			for (size_t i = 0, e = points->size(); i < e; ++i) {
				auto &point = (*points)[i];
				const auto x = point.x;
				const auto y = point.x;
				const auto z = point.x;
				auto output = 0.0f;

				for (size_t j = 0; j < octaves; ++j) {
					auto frequency = ofrequency;
					auto amplitude = oamplitude;

					output += (amplitude * simplex_noise_3d(x * frequency, y * frequency, z * frequency));

					frequency *= lacunarity;
					amplitude *= persistence;
				}

				point.x += output;
				point.y += output;
				point.z += output;
			}
		}
	}
};

/* ************************************************************************** */

enum {
	COLOR_NODE_VERTEX    = 0,
	COLOR_NODE_PRIMITIVE = 1,
};

enum {
	COLOR_NODE_UNIQUE = 0,
	COLOR_NODE_RANDOM = 1,
};

class ColorNode : public Node {
public:
	ColorNode()
	    : Node("Color")
	{
		addInput("input");
		addOutput("output");

		EnumProperty scope_enum_prop;
		scope_enum_prop.insert("Vertex", COLOR_NODE_VERTEX);
		scope_enum_prop.insert("Primitive", COLOR_NODE_PRIMITIVE);

		add_prop("scope", "Scope", property_type::prop_enum);
		set_prop_enum_values(scope_enum_prop);

		EnumProperty color_enum_prop;
		color_enum_prop.insert("Unique", COLOR_NODE_UNIQUE);
		color_enum_prop.insert("Random", COLOR_NODE_RANDOM);

		add_prop("fill_method", "Fill Method", property_type::prop_enum);
		set_prop_enum_values(color_enum_prop);

		add_prop("color", "Color", property_type::prop_vec3);
		set_prop_min_max(0.0f, 1.0f);
		set_prop_default_value_vec3(glm::vec3{1.0f, 1.0f, 1.0f});

		add_prop("seed", "Seed", property_type::prop_int);
		set_prop_min_max(1, 100000);
		set_prop_default_value_int(1);
	}

	bool update_properties() override
	{
		auto method = eval_int("fill_method");

		if (method == COLOR_NODE_UNIQUE) {
			set_prop_visible("color", true);
			set_prop_visible("seed", false);
		}
		else if (method == COLOR_NODE_RANDOM) {
			set_prop_visible("seed", true);
			set_prop_visible("color", false);
		}

		return true;
	}

	void process() override
	{
		const auto &method = eval_int("fill_method");
		const auto &scope = eval_int("scope");
		const auto &seed = eval_int("seed");

		std::mt19937 rng(19937 + seed);
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		for (auto prim : primitive_iterator(this->m_collection)) {
			Attribute *colors;

			if (prim->typeID() == Mesh::id) {
				auto mesh = static_cast<Mesh *>(prim);
				colors = mesh->add_attribute("color", ATTR_TYPE_VEC3, mesh->points()->size());
			}
			else if (prim->typeID() == PrimPoints::id) {
				auto prim_points = static_cast<PrimPoints *>(prim);
				colors = prim_points->add_attribute("color", ATTR_TYPE_VEC3, prim_points->points()->size());
			}
			else {
				continue;
			}

			if (method == COLOR_NODE_UNIQUE) {
				const auto &color = eval_vec3("color");

				for (size_t i = 0, e = colors->size(); i < e; ++i) {
					colors->vec3(i, color);
				}
			}
			else if (method == COLOR_NODE_RANDOM) {
				if (scope == COLOR_NODE_VERTEX) {
					for (size_t i = 0, e = colors->size(); i < e; ++i) {
						colors->vec3(i, glm::vec3{dist(rng), dist(rng), dist(rng)});
					}
				}
				else if (scope == COLOR_NODE_PRIMITIVE) {
					const auto &color = glm::vec3{dist(rng), dist(rng), dist(rng)};

					for (size_t i = 0, e = colors->size(); i < e; ++i) {
						colors->vec3(i, color);
					}
				}
			}
		}
	}
};

/* ************************************************************************** */

class CollectionMergeNode : public Node {
public:
	CollectionMergeNode()
	    : Node("Merge Collection")
	{
		addInput("input1");
		addInput("input2");
		addOutput("output");
	}

	void process() override
	{
		auto collection2 = this->getInputCollection("input2");

		if (collection2 == nullptr) {
			return;
		}

		m_collection->merge_collection(*collection2);
	}
};

/* ************************************************************************** */

class CreatePointCloudNode : public Node {
public:
	CreatePointCloudNode()
	    : Node("Point Cloud")
	{
		addOutput("Primitive");

		add_prop("points_count", "Points Count", property_type::prop_int);
		set_prop_min_max(1, 100000);
		set_prop_default_value_int(1000);

		add_prop("bbox_min", "BBox Min", property_type::prop_vec3);
		set_prop_min_max(-10.0f, 10.0f);
		set_prop_default_value_vec3(glm::vec3{-1.0f, -1.0f, -1.0f});

		add_prop("bbox_max", "BBox Max", property_type::prop_vec3);
		set_prop_min_max(-10.0f, 10.0f);
		set_prop_default_value_vec3(glm::vec3{1.0f, 1.0f, 1.0f});
	}

	void process() override
	{
		auto prim = m_collection->build("PrimPoints");
		auto points = static_cast<PrimPoints *>(prim);

		const auto &point_count = eval_int("points_count");

		auto point_list = points->points();
		point_list->resize(point_count);

		const auto &bbox_min = eval_vec3("bbox_min");
		const auto &bbox_max = eval_vec3("bbox_max");

		std::uniform_real_distribution<float> dist_x(bbox_min[0], bbox_max[0]);
		std::uniform_real_distribution<float> dist_y(bbox_min[1], bbox_max[1]);
		std::uniform_real_distribution<float> dist_z(bbox_min[2], bbox_max[2]);
		std::mt19937 rng_x(19937);
		std::mt19937 rng_y(19937 + 1);
		std::mt19937 rng_z(19937 + 2);

		for (size_t i = 0; i < point_count; ++i) {
			const auto &point = glm::vec3(dist_x(rng_x), dist_y(rng_y), dist_z(rng_z));
			(*point_list)[i] = point;
		}

		points->tagUpdate();
	}
};

/* ************************************************************************** */

class CreateAttributeNode : public Node {
public:
	CreateAttributeNode()
	    : Node("Attribute Create")
	{
		addInput("input");
		addOutput("output");

		add_prop("attribute_name", "Name", property_type::prop_string);
		set_prop_tooltip("Name of the attribute to create.");

		EnumProperty type_enum;
		type_enum.insert("Byte", ATTR_TYPE_BYTE);
		type_enum.insert("Integer", ATTR_TYPE_INT);
		type_enum.insert("Float", ATTR_TYPE_FLOAT);
		type_enum.insert("String", ATTR_TYPE_STRING);
		type_enum.insert("2D Vector", ATTR_TYPE_VEC2);
		type_enum.insert("3D Vector", ATTR_TYPE_VEC3);
		type_enum.insert("4D Vector", ATTR_TYPE_VEC4);
		type_enum.insert("3x3 Matrix", ATTR_TYPE_MAT3);
		type_enum.insert("4x4 Matrix", ATTR_TYPE_MAT4);

		add_prop("attribute_type", "Attribute Type", property_type::prop_enum);
		set_prop_enum_values(type_enum);
	}

	void process() override
	{
		auto name = eval_string("attribute_name");
		auto attribute_type = static_cast<AttributeType>(eval_enum("attribute_type"));

		for (Primitive *prim : primitive_iterator(m_collection)) {
			if (prim->has_attribute(name, attribute_type)) {
				std::stringstream ss;
				ss << prim->name() << " already has an attribute named " << name;

				this->add_warning(ss.str());
				continue;
			}

			size_t attrib_size = 1;

			if (prim->typeID() == Mesh::id) {
				auto mesh = static_cast<Mesh *>(prim);

				if (attribute_type == ATTR_TYPE_VEC3) {
					attrib_size = mesh->points()->size();
				}
			}
			else if (prim->typeID() == PrimPoints::id) {
				auto point_cloud = static_cast<PrimPoints *>(prim);

				if (attribute_type == ATTR_TYPE_VEC3) {
					attrib_size = point_cloud->points()->size();
				}
			}

			prim->add_attribute(name, attribute_type, attrib_size);
		}
	}
};

/* ************************************************************************** */

class DeleteAttributeNode : public Node {
public:
	DeleteAttributeNode()
	    : Node("Attribute Delete")
	{
		addInput("input");
		addOutput("output");

		add_prop("attribute_name", "Name", property_type::prop_string);
		set_prop_tooltip("Name of the attribute to delete.");

		EnumProperty type_enum;
		type_enum.insert("Byte", ATTR_TYPE_BYTE);
		type_enum.insert("Integer", ATTR_TYPE_INT);
		type_enum.insert("Float", ATTR_TYPE_FLOAT);
		type_enum.insert("String", ATTR_TYPE_STRING);
		type_enum.insert("2D Vector", ATTR_TYPE_VEC2);
		type_enum.insert("3D Vector", ATTR_TYPE_VEC3);
		type_enum.insert("4D Vector", ATTR_TYPE_VEC4);
		type_enum.insert("3x3 Matrix", ATTR_TYPE_MAT3);
		type_enum.insert("4x4 Matrix", ATTR_TYPE_MAT4);

		add_prop("attribute_type", "Attribute Type", property_type::prop_enum);
		set_prop_enum_values(type_enum);
	}

	void process() override
	{
		auto name = eval_string("attribute_name");
		auto attribute_type = static_cast<AttributeType>(eval_enum("attribute_type"));

		for (Primitive *prim : primitive_iterator(m_collection)) {
			if (!prim->has_attribute(name, attribute_type)) {
				continue;
			}

			prim->remove_attribute(name, attribute_type);
		}
	}
};

/* ************************************************************************** */

enum {
	DIST_CONSTANT = 0,
	DIST_UNIFORM,
	DIST_GAUSSIAN,
	DIST_EXPONENTIAL,
	DIST_LOGNORMAL,
	DIST_CAUCHY,
	DIST_DISCRETE,
};

class RandomiseAttributeNode : public Node {
public:
	RandomiseAttributeNode()
	    : Node("Attribute Randomise")
	{
		addInput("input");
		addOutput("output");

		add_prop("attribute_name", "Name", property_type::prop_string);
		set_prop_tooltip("Name of the attribute to randomise.");

		EnumProperty type_enum;
		type_enum.insert("Byte", ATTR_TYPE_BYTE);
		type_enum.insert("Integer", ATTR_TYPE_INT);
		type_enum.insert("Float", ATTR_TYPE_FLOAT);
		type_enum.insert("String", ATTR_TYPE_STRING);
		type_enum.insert("2D Vector", ATTR_TYPE_VEC2);
		type_enum.insert("3D Vector", ATTR_TYPE_VEC3);
		type_enum.insert("4D Vector", ATTR_TYPE_VEC4);
		type_enum.insert("3x3 Matrix", ATTR_TYPE_MAT3);
		type_enum.insert("4x4 Matrix", ATTR_TYPE_MAT4);

		add_prop("attribute_type", "Attribute Type", property_type::prop_enum);
		set_prop_enum_values(type_enum);

		EnumProperty dist_enum;
		dist_enum.insert("Constant", DIST_CONSTANT);
		dist_enum.insert("Uniform", DIST_UNIFORM);
		dist_enum.insert("Gaussian", DIST_GAUSSIAN);

		add_prop("distribution", "Distribution", property_type::prop_enum);
		set_prop_enum_values(dist_enum);

		/* Constant Value */
		add_prop("value", "Value", property_type::prop_float);
		set_prop_min_max(0.0f, 10.0f);
		set_prop_default_value_float(1.0f);

		/* Min/Max Value */
		add_prop("min_value", "Min Value", property_type::prop_float);
		set_prop_min_max(0.0f, 10.0f);
		set_prop_default_value_float(0.0f);

		add_prop("max_value", "Max Value", property_type::prop_float);
		set_prop_min_max(0.0f, 10.0f);
		set_prop_default_value_float(1.0f);

		/* Gaussian Distribution */
		add_prop("mean", "Mean", property_type::prop_float);
		set_prop_min_max(0.0f, 10.0f);
		set_prop_default_value_float(0.0f);

		add_prop("stddev", "Standard Deviation", property_type::prop_float);
		set_prop_min_max(0.0f, 10.0f);
		set_prop_default_value_float(1.0f);
	}

	bool update_properties() override
	{
		const auto distribution = eval_enum("distribution");

		set_prop_visible("value", distribution == DIST_CONSTANT);
		set_prop_visible("min_value", distribution == DIST_UNIFORM);
		set_prop_visible("max_value", distribution == DIST_UNIFORM);
		set_prop_visible("mean", distribution == DIST_GAUSSIAN);
		set_prop_visible("stddev", distribution == DIST_GAUSSIAN);

		return true;
	}

	void process() override
	{
		auto name = eval_string("attribute_name");
		auto attribute_type = static_cast<AttributeType>(eval_enum("attribute_type"));
		auto distribution = eval_enum("distribution");
		auto value = eval_float("value");
		auto min_value = eval_float("min_value");
		auto max_value = eval_float("max_value");
		auto mean = eval_float("mean");
		auto stddev = eval_float("stddev");

		std::mt19937 rng(19993754);

		if (attribute_type != ATTR_TYPE_VEC3) {
			std::stringstream ss;
			ss << "Only 3D Vector attributes are supported for now!";

			this->add_warning(ss.str());
			return;
		}

		for (Primitive *prim : primitive_iterator(m_collection)) {
			auto attribute = prim->attribute(name, attribute_type);

			if (!attribute) {
				std::stringstream ss;
				ss << prim->name() << " does not have an attribute named \"" << name
				   << "\" of type " << static_cast<int>(attribute_type);

				this->add_warning(ss.str());
				continue;
			}

			switch (distribution) {
				case DIST_CONSTANT:
				{
					for (size_t i = 0; i < attribute->size(); ++i) {
						attribute->vec3(i, glm::vec3{value, value, value});
					}

					break;
				}
				case DIST_UNIFORM:
				{
					std::uniform_real_distribution<float> dist(min_value, max_value);

					for (size_t i = 0; i < attribute->size(); ++i) {
						attribute->vec3(i, glm::vec3{dist(rng), dist(rng), dist(rng)});
					}

					break;
				}
				case DIST_GAUSSIAN:
				{
					std::normal_distribution<float> dist(mean, stddev);

					for (size_t i = 0; i < attribute->size(); ++i) {
						attribute->vec3(i, glm::vec3{dist(rng), dist(rng), dist(rng)});
					}

					break;
				}
			}
		}
	}
};

/* ************************************************************************** */

#include <kamikaze/segmentprim.h>

class FurNode : public Node {
public:
	FurNode()
	    : Node("Fur")
	{
		addInput("input");
		addOutput("output");

		add_prop("segment", "Segment", property_type::prop_int);
		set_prop_default_value_int(1);
		set_prop_min_max(1, 10);
		set_prop_tooltip("Number of segments in each generated curve.");

		add_prop("size", "Size", property_type::prop_float);
		set_prop_default_value_float(1);
		set_prop_min_max(0.0f, 10.0f);
		set_prop_tooltip("Size of each segment.");

		add_prop("normal", "Normal", property_type::prop_vec3);
		set_prop_default_value_vec3(glm::vec3{0.0f, 1.0f, 0.0f});
		set_prop_min_max(-1.0f, 1.0f);
		set_prop_tooltip("Direction of the generated curves.");
	}

	void process() override
	{
		if (!getInputCollection("input")) {
			return;
		}

		auto iter = primitive_iterator(m_collection, Mesh::id);

		if (iter.get() == nullptr) {
			this->add_warning("No input mesh found!");
			return;
		}

		auto input_mesh = static_cast<Mesh *>(iter.get());
		auto input_points = input_mesh->points();

		const auto segment_number = eval_int("segment");
		const auto segment_normal = eval_vec3("normal");
		const auto segment_size = eval_float("size");

		auto segment_prim = static_cast<SegmentPrim *>(m_collection->build("SegmentPrim"));
		auto output_edges = segment_prim->edges();
		output_edges->reserve(input_points->size() * segment_number);

		auto output_points = segment_prim->points();
		auto total_points = input_points->size() * (segment_number + 1);
		output_points->reserve(total_points);

		auto num_points = 0;
		auto head = 0;

		for (size_t i = 0; i < input_points->size(); ++i) {
			auto point = (*input_points)[i];

			output_points->push_back(point);
			++num_points;

			for (int j = 0; j < segment_number; ++j, ++num_points) {
				point += (segment_size * segment_normal);
				output_points->push_back(point);

				output_edges->push_back(glm::uvec2{head, ++head});
			}

			++head;
		}

		if (num_points != total_points) {
			std::stringstream ss;

			if (num_points < total_points) {
				ss << "Overallocation of points, allocated: " << total_points
				   << ", final total: " << num_points;
			}
			else if (num_points > total_points) {
				ss << "Underallocation of points, allocated: " << total_points
				   << ", final total: " << num_points;
			}

			this->add_warning(ss.str());
		}
	}
};

/* ************************************************************************** */

void register_builtin_nodes(NodeFactory *factory)
{
	REGISTER_NODE("Geometry", "Box", CreateBoxNode);
	REGISTER_NODE("Geometry", "Grid", CreateGridNode);
	REGISTER_NODE("Geometry", "Torus", CreateTorusNode);
	REGISTER_NODE("Geometry", "Transform", TransformNode);
	REGISTER_NODE("Geometry", "Circle", CreateCircleNode);
	REGISTER_NODE("Geometry", "Tube", CreateTubeNode);
	REGISTER_NODE("Geometry", "IcoSphere", CreateIcoSphereNode);
	REGISTER_NODE("Geometry", "Cone", CreateConeNode);
	REGISTER_NODE("Geometry", "Noise", NoiseNode);
	REGISTER_NODE("Geometry", "Normal", NormalNode);
	REGISTER_NODE("Geometry", "Color", ColorNode);
	REGISTER_NODE("Geometry", "Merge Collection", CollectionMergeNode);
	REGISTER_NODE("Geometry", "Point Cloud", CreatePointCloudNode);
	REGISTER_NODE("Geometry", "Fur", FurNode);

	REGISTER_NODE("Attribute", "Attribute Create", CreateAttributeNode);
	REGISTER_NODE("Attribute", "Attribute Delete", DeleteAttributeNode);
	REGISTER_NODE("Attribute", "Attribute Randomise", RandomiseAttributeNode);
}
