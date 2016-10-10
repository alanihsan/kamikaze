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

#include "undo.h"

#include <cstring>

#include "context.h"

#include "util/util_input.h"
#include "util/util_memory.h"

/* ************************************************************************** */

void Command::setName(const std::string &name)
{
	m_name = name;
}

CommandManager::~CommandManager()
{
	release_stack_memory(m_undo_commands);
	release_stack_memory(m_redo_commands);
}

void CommandManager::execute(Command *command, const Context &context)
{
	command->execute(context);
	m_undo_commands.push(command);
}

static void undo_redo_ex(std::stack<Command *> &pop_stack,
                         std::stack<Command *> &push_stack,
                         bool redo)
{
	if (pop_stack.empty()) {
		return;
	}

	auto command = pop_stack.top();
	pop_stack.pop();

	if (redo) {
		command->redo();
	}
	else {
		command->undo();
	}

	push_stack.push(command);
}

void CommandManager::undo()
{
	undo_redo_ex(m_undo_commands, m_redo_commands, false);
}

void CommandManager::redo()
{
	undo_redo_ex(m_redo_commands, m_undo_commands, true);
}

/* ************************************************************************** */

#include "camera.h"
#include "util/utils_glm.h"

class CameraPanCommand : public Command {
	int m_old_x = 0;
	int m_old_y = 0;

public:
	void undo() override {}
	void redo() override {}

	void invoke(const Context &context) override
	{
		const auto view_3d = context.view_3d;
		m_old_x = view_3d->x;
		m_old_y = view_3d->y;
	}

	void execute(const Context &context) override
	{
		const auto view_3d = context.view_3d;
		const auto x = view_3d->x;
		const auto y = view_3d->y;
		const auto dx = static_cast<float>(x - m_old_x);
		const auto dy = static_cast<float>(y - m_old_y);

		const auto &camera = view_3d->camera;
		const auto &up = camera->up();
		const auto &right = camera->right();
		const auto &strafe_speed = camera->strafe_speed();

		auto center = camera->center();
		center += ((dy * up - dx * right) * strafe_speed);

		camera->center(center);

		camera->tag_update();

		m_old_x = x;
		m_old_y = y;
	}

	bool modal() const override { return true; }
};

class CameraTumbleCommand : public Command {
	int m_old_x = 0;
	int m_old_y = 0;

public:
	void undo() override {}
	void redo() override {}

	void invoke(const Context &context) override
	{
		const auto view_3d = context.view_3d;
		m_old_x = view_3d->x;
		m_old_y = view_3d->y;
	}

	void execute(const Context &context) override
	{
		const auto view_3d = context.view_3d;
		const auto x = view_3d->x;
		const auto y = view_3d->y;
		const auto dx = static_cast<float>(x - m_old_x);
		const auto dy = static_cast<float>(y - m_old_y);

		const auto &camera = view_3d->camera;
		const auto &tumbling_speed = camera->tumbling_speed();

		camera->head(camera->head() + dy * tumbling_speed);
		camera->pitch(camera->pitch() + dx * tumbling_speed);

		camera->tag_update();

		m_old_x = x;
		m_old_y = y;
	}

	bool modal() const override { return true; }
};

class CameraZoomInCommand : public Command {
public:
	void undo() override {}
	void redo() override {}

	void execute(const Context &context) override
	{
		const auto view_3d = context.view_3d;
		const auto &camera = view_3d->camera;

		camera->distance(camera->distance() + camera->zoom_speed());
		camera->set_speed();
		camera->tag_update();
	}
};

class CameraZoomOutCommand : public Command {
public:
	void undo() override {}
	void redo() override {}

	void execute(const Context &context) override
	{
		const auto view_3d = context.view_3d;
		const auto &camera = view_3d->camera;

		camera->distance(glm::max(0.0f, camera->distance() - camera->zoom_speed()));
		camera->set_speed();
		camera->tag_update();
	}
};

namespace KeyEventHandler {

static CommandManager command_manager;
static Command *modal_command = nullptr;
static std::vector<KeyData> keys;

void init_key_mappings()
{
	keys.emplace_back(0, 0, "add node");
	keys.emplace_back(0, 0, "add object");
	keys.emplace_back(0, 0, "add preset");
	keys.emplace_back(MOD_KEY_NONE, MOUSE_MIDDLE, "CameraTumbleCommand");
	keys.emplace_back(MOD_KEY_SHIFT, MOUSE_MIDDLE, "CameraPanCommand");
	keys.emplace_back(MOD_KEY_NONE, MOUSE_SCROLL_DOWN, "CameraZoomOutCommand");
	keys.emplace_back(MOD_KEY_NONE, MOUSE_SCROLL_UP, "CameraZoomInCommand");
	keys.emplace_back(MOD_KEY_NONE, 0x01000007, "DeleteObjectCommand");
}

void call_command(const Context &context, const KeyData &key_data, const std::string &name)
{
	Command *command = nullptr;

	for (const KeyData &key : keys) {
		if (key.key != key_data.key) {
			continue;
		}

		if (key.modifier != key_data.modifier) {
			continue;
		}

		/* TODO */
		if (key_data.command) {
			if (std::strcmp(key.command, key_data.command) != 0) {
				continue;
			}
		}

		if (!context.command_factory->registered(key.command)) {
			continue;
		}

		command = (*context.command_factory)(key.command);
		break;
	}

	if (command == nullptr) {
		return;
	}

	/* TODO: find a way to pass custom datas. */
	command->setName(name);

	if (command->modal()) {
		modal_command = command;
		modal_command->invoke(context);
	}
	else {
		command_manager.execute(command, context);
	}
}

void call_modal_command(const Context &context)
{
	if (modal_command == nullptr) {
		return;
	}

	modal_command->execute(context);
}

void end_modal_command()
{
	delete modal_command;
	modal_command = nullptr;
}

void undo()
{
	/* TODO: figure out how to update everything properly */
	command_manager.undo();
}

void redo()
{
	/* TODO: figure out how to update everything properly */
	command_manager.redo();
}

}  /* namespace KeyEventHandler */

void register_commands(CommandFactory *factory)
{
	REGISTER_COMMAND(factory, "CameraTumbleCommand", CameraTumbleCommand);
	REGISTER_COMMAND(factory, "CameraPanCommand", CameraPanCommand);
	REGISTER_COMMAND(factory, "CameraZoomInCommand", CameraZoomInCommand);
	REGISTER_COMMAND(factory, "CameraZoomOutCommand", CameraZoomOutCommand);
}
