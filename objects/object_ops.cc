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

#include "object_ops.h"

#include <openvdb/tools/LevelSetSphere.h>
#include <openvdb/tools/LevelSetUtil.h>

#include "levelset.h"
#include "volume.h"

#include "../render/scene.h"
#include "../util/util_openvdb.h"

void add_object(Scene *scene, const QString &name, int type, float radius,
                float voxel_size, float halfwidth)
{
	using namespace openvdb;
	using namespace openvdb::math;

	Object *ob = nullptr;

	switch (type) {
		case OBJECT_CUBE:
		{
			glm::vec3 min(-1.0f), max(1.0f);
			ob = new Cube(min * radius, max * radius);
			break;
		}
		case OBJECT_SPHERE_LS:
		{
			FloatGrid::Ptr ls = tools::createLevelSetSphere<FloatGrid>(
			                        radius, Vec3f(0.0f), voxel_size, halfwidth);
			ob = new LevelSet(ls->deepCopy());
			break;
		}
		case OBJECT_CUBE_LS:
		{
			Transform xform = *Transform::createLinearTransform(voxel_size);
			Vec3s min(-1.0f * radius), max(1.0f * radius);
			BBox<math::Vec3s> bbox(min, max);

			FloatGrid::Ptr ls = tools::createLevelSetBox<FloatGrid>(bbox, xform, halfwidth);
			ob = new LevelSet(ls->deepCopy());
			break;
		}
		default:
			return;
	}

	assert(ob != nullptr);

	ob->name(name);
	scene->addObject(ob);
}

/* *************************** add object command *************************** */

AddObjectCmd::AddObjectCmd(Scene *scene, const QString &name, int type, float radius, float voxel_size, float halfwidth)
    : m_object(nullptr)
    , m_scene(scene)
    , m_name(name)
    , m_type(type)
    , m_radius(radius)
    , m_voxel_size(voxel_size)
    , m_halfwidth(halfwidth)
    , m_was_undone(false)
{}

AddObjectCmd::~AddObjectCmd()
{
	if (m_was_undone) {
		delete m_object;
	}
}

void AddObjectCmd::execute()
{
	using namespace openvdb;
	using namespace openvdb::math;

	switch (m_type) {
		case OBJECT_CUBE:
		{
			glm::vec3 min(-1.0f), max(1.0f);
			m_object = new Cube(min * m_radius, max * m_radius);
			break;
		}
		case OBJECT_SPHERE_LS:
		{
			FloatGrid::Ptr ls = tools::createLevelSetSphere<FloatGrid>(
			                        m_radius, Vec3f(0.0f), m_voxel_size, m_halfwidth);
			m_object = new LevelSet(ls->deepCopy());
			break;
		}
		case OBJECT_CUBE_LS:
		{
			Transform xform = *Transform::createLinearTransform(m_voxel_size);
			Vec3s min(-1.0f * m_radius), max(1.0f * m_radius);
			BBox<math::Vec3s> bbox(min, max);

			FloatGrid::Ptr ls = tools::createLevelSetBox<FloatGrid>(bbox, xform, m_halfwidth);
			m_object = new LevelSet(ls->deepCopy());
			break;
		}
		default:
			return;
	}

	assert(m_object != nullptr);

	m_object->name(m_name);
	m_scene->addObject(m_object);
}

void AddObjectCmd::undo()
{
	m_scene->removeObject(m_object);
	m_was_undone = true;
}

void AddObjectCmd::redo()
{
	m_scene->addObject(m_object);
	m_was_undone = false;
}

/* *************************** load object command ************************** */

LoadFromFileCmd::LoadFromFileCmd(Scene *scene, const QString &filename)
    : m_scene(scene)
    , m_object(nullptr)
    , m_filename(filename)
    , m_was_undone(false)
{}

LoadFromFileCmd::~LoadFromFileCmd()
{
	if (m_was_undone) {
		delete m_object;
	}
}

void LoadFromFileCmd::execute()
{
	using openvdb::FloatGrid;
	using openvdb::Vec3s;

	openvdb::initialize();
	openvdb::io::File file(m_filename.toStdString());

	if (!file.open()) {
		std::cerr << "Unable to open file \'" << m_filename.toStdString() << "\'\n";
		return;
	}

	FloatGrid::Ptr grid;

	if (file.hasGrid(openvdb::Name("Density"))) {
		grid = openvdb::gridPtrCast<FloatGrid>(file.readGrid(openvdb::Name("Density")));
	}
	else if (file.hasGrid(openvdb::Name("density"))) {
		grid = openvdb::gridPtrCast<FloatGrid>(file.readGrid(openvdb::Name("density")));
	}
	else {
		openvdb::GridPtrVecPtr grids = file.getGrids();
		grid = openvdb::gridPtrCast<FloatGrid>((*grids)[0]);
	}

	auto meta_map = file.getMetadata();

	file.close();

	if ((*meta_map)["creator"]) {
		auto creator = (*meta_map)["creator"]->str();

		/* If the grid comes from Blender (Z-up), rotate it so it is Y-up */
		if (creator == "Blender/Smoke") {
			Timer("Transform Blender Grid");
			grid = transform_grid(*grid, Vec3s(-M_PI_2, 0.0f, 0.0f),
			                      Vec3s(1.0f), Vec3s(0.0f), Vec3s(0.0f));
		}
	}

	if (grid->getGridClass() == openvdb::GRID_LEVEL_SET) {
		m_object = new LevelSet(grid);
	}
	else {
		m_object = new Volume(grid);
	}

	m_object->name(grid->getName().c_str());
	m_scene->addObject(m_object);
}

void LoadFromFileCmd::undo()
{
	m_scene->removeObject(m_object);
	m_was_undone = true;
}

void LoadFromFileCmd::redo()
{
	m_scene->addObject(m_object);
	m_was_undone = false;
}