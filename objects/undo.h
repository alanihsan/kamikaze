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

#pragma once

#include <stack>
#include <unordered_map>

#include "factory.h"
#include "ui/paramfactory.h"

class EvaluationContext;

class Command {
public:
	virtual ~Command() = default;

	virtual void execute(EvaluationContext *context) = 0;
	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual void setUIParams(ParamCallback &cb) = 0;
};

class CommandManager final {
	std::stack<Command *> m_undo_commands;
	std::stack<Command *> m_redo_commands;

public:
	~CommandManager();

	void execute(Command *command, EvaluationContext *context);
	void undo();
	void redo();
};

using CommandFactory = Factory<Command>;
