/*
 * vu|KNOB
 * Copyright (C) 2015 by Anton Persson
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

#ifndef LINKED_LIST_HH
#define LINKED_LIST_HH

#include <functional>

template <class T>
class LinkedList {
public:
	T* head;

	LinkedList() : head(NULL) {}

	void insert_element(T* new_element) {
		if(head == NULL || (*new_element) < (*head)) {
			new_element->next = head;
			head = new_element;
		} else {
			T* this_element = head;
			while(this_element->next != NULL &&
			      (*this_element) < (*new_element)) {
				this_element = this_element->next;
			}
			new_element->next = this_element->next;
			this_element->next = new_element;
		}
	}

	void clear(std::function<void(T*)> func_on_drop) {
		while(head) {
			T* to_drop = head;
			head = head->next;
			func_on_drop(to_drop);
		}
	}

	void drop_element(const T* element_to_drop, std::function<void(T*)> func_on_drop) {
		if(head == NULL) return;

		T* element_found = NULL;

		if(head == element_to_drop) {
			element_found = head;
			head = head->next;
		} else {
			T* this_element = head;
			while(this_element->next != NULL &&
			      *(this_element->next) != *element_to_drop) {
				this_element = this_element->next;
			}
			if(this_element &&
			   this_element->next &&
			   *(this_element->next) == *element_to_drop) {
				element_found = this_element->next;
				this_element->next = element_found->next;
			}
		}

		if(element_found) func_on_drop(element_found);
	}

	void for_each(std::function<void(T*)> func_on_element) {
		T* current = head;
		while(current != NULL) {
			func_on_element(current);
			current = current->next;
		}
	}
};

#endif
