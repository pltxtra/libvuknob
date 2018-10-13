/*
 * vu|KNOB
 * (C) 2014 by Anton Persson
 *
 * http://www.vuknob.com/
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License version 2; see COPYING for the complete License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef COMMON_HH
#define COMMON_HH

#include <set>
#include <vector>
#include <stack>
#include <stdint.h>
#include <stdexcept>

/***************************
 *
 * Useful macros and constants
 *
 ***************************/
#define QUALIFIED_DELETE(a) {if(a) { delete a; a = NULL; }}
#define INCHES_PER_FINGER 0.31f
#define FLING_DEACCELERATION 2000.0f
#define TRANSITION_TIME 0.125

#define NOT_USED(a)

#ifdef ANDROID
extern const char* KAMOFLAGE_ANDROID_ROOT_DIRECTORY;
extern const char* DEFAULT_PROJECT_SAVE_PATH;
extern const char* DEFAULT_SATAN_ROOT;
extern const char* DEFAULT_EXPORT_PATH;
#endif

class IDAllocator {
private:
	std::set<uint32_t> available_ids;

	uint32_t total_amount;
public:
	static constexpr uint32_t NO_ID_AVAILABLE = 0xffffffff;

	class IDFreedTwice : public std::runtime_error {
	public:
		IDFreedTwice() : runtime_error("Tried to free an ID twice.") {}
		virtual ~IDFreedTwice() {}
	};

	IDAllocator(uint32_t initial_size = 32) : total_amount(initial_size) {
		for(uint32_t x = 0; x < initial_size; x++) {
			available_ids.insert(x);
		}
	}

	uint32_t get_id() {
		uint32_t retval = total_amount;

		if(available_ids.size() > 0) {
			auto iter = available_ids.begin();
			retval = *iter;
			available_ids.erase(iter);
		} else total_amount++;

		return retval;
	}

	void free_id(uint32_t id) {
		if(available_ids.find(id) != available_ids.end())
			throw IDFreedTwice();

		available_ids.insert(id);
	}
};

template <class T>
class ObjectAllocator {
private:
	std::stack<T*> free_objects;

public:
	T* allocate() {
		T* retval = NULL;
		if(free_objects.size() > 0) {
			retval = free_objects.top();
			free_objects.pop();
		} else {
			retval = new T();
		}

		return retval;
	}

	void recycle(T* obj) {
		free_objects.push(obj);
	}
};

/**************************
 *
 * Replace missing C++11 functionality
 * in the Android NDK (r10d latest checked)
 *
 */

#ifdef ANDROID

#include <string>
#include <sstream>
#include <stdlib.h>

namespace std {

	template <typename T>
	inline string to_string(T value) {
		ostringstream os ;
		os << value ;
		return os.str() ;
	}

	inline long stol(const string &str) {
		return atol(str.c_str());
	}

	inline int stoi(const string &str) {
		return atoi(str.c_str());
	}

	inline float stof(const string &str) {
		return strtof(str.c_str(), 0);
	}

	inline double stod(const string &str) {
		return strtod(str.c_str(), 0);
	}

	inline unsigned long stoul(const string &str) {
		return strtoul(str.c_str(), 0, 0);
	}
};

#endif

#endif // COMMON_HH
