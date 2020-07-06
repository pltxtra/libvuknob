/*
 * vu|KNOB
 * Copyright (C) 2020 by Anton Persson
 *
 * http://www.vuknob.com/
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include<stack>
#include<memory>
#include<sstream>

#ifndef VUKNOB_UI_STACK
#define VUKNOB_UI_STACK

class AbstractHideable {
public:
	virtual void show() = 0;
	virtual void hide() = 0;
};

template <class H>
class Hideable : public AbstractHideable {
private:
	std::shared_ptr<H> i;

public:
	Hideable(std::shared_ptr<H> h) : i(h) {}
	virtual void show() override {
		i->show();
	}
	virtual void hide() override {
		i->hide();
	}
};

class UIStack {
private:
	static std::shared_ptr<AbstractHideable> current;
	static std::stack<std::shared_ptr<AbstractHideable> > previous;

	static void print_string(const std::string& str);
public:
	template <class T>
	static void push(std::shared_ptr<T> i, bool already_visible = false) {
		std::stringstream ss;
		ss << "::push(" << (already_visible ? "true" : "false") << ")\n";
		print_string(ss.str());

		auto next = std::make_shared<Hideable<T> >(i);
		if(current) {
			current->hide();
			previous.push(current);
		}
		if(!already_visible)
			next->show();
		current = next;
	}

	static void pop() {
		print_string("::pop() begin\n");
		if(current) {
			current->hide();
			current.reset();
		}
		print_string("::pop() middle\n");
		if(!previous.empty()) {
			print_string("::pop() middle A\n");
			current = previous.top();
			print_string("::pop() middle B\n");
			previous.pop();
			print_string("::pop() middle C\n");
			current->show();
			print_string("::pop() middle D\n");
		}
		print_string("::pop() end\n");
	}

	static void clear() {
		if(current) {
			current->hide();
			current.reset();
		}
		while (!previous.empty()) {
			previous.pop();
		}
	}
};

#endif