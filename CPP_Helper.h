#pragma once

#include <type_traits>

#define CLASS_COPY_FUNC(class_name, keyword)\
class_name(const class_name &_Copy) = keyword;\
class_name &operator=(const class_name &_Copy) = keyword;

#define CLASS_MOVE_FUNC(class_name, keyword)\
class_name(class_name &&_Move) = keyword;\
class_name &operator=(class_name &&_Move) = keyword;

#define CLASS_DESTRUCTOR(class_name, keyword)\
~class_name(void) = keyword;


#define DEFAULT_COPY(class_name)\
CLASS_COPY_FUNC(class_name, default)

#define DEFAULT_MOVE(class_name)\
CLASS_MOVE_FUNC(class_name, default)

#define DEFAULT_DSTC(class_name)\
CLASS_DESTRUCTOR(class_name, default)


#define DELETE_COPY(class_name)\
CLASS_COPY_FUNC(class_name, delete)

#define DELETE_MOVE(class_name)\
CLASS_MOVE_FUNC(class_name, delete)

#define DELETE_DSTC(class_name)\
CLASS_DESTRUCTOR(class_name, delete)


#define VIRTUAL_COPY(class_name)\
CLASS_COPY_FUNC(class_name, 0)

#define VIRTUAL_MOVE(class_name)\
CLASS_MOVE_FUNC(class_name, 0)

#define VIRTUAL_DSTC(class_name)\
CLASS_DESTRUCTOR(class_name, 0)


#define DEFAULT_CPMV(class_name)\
DEFAULT_COPY(class_name)\
DEFAULT_MOVE(class_name)

#define DELETE_CPMV(class_name)\
DELETE_COPY(class_name)\
DELETE_MOVE(class_name)

#define VIRTUAL_CPMV(class_name)\
VIRTUAL_COPY(class_name)\
VIRTUAL_MOVE(class_name)


#define GETTER(func_name, member_name)\
typename std::remove_reference_t<decltype(member_name)> &Get##func_name(void) noexcept\
{\
	return member_name;\
}\
const typename std::remove_reference_t<decltype(member_name)> &Get##func_name(void) const noexcept\
{\
	return member_name;\
}


#define SETTER(func_name, member_name)\
void Set##func_name(typename std::remove_reference_t<decltype(member_name)> &_member_name)\
{\
	member_name = _member_name;\
}\
void Set##func_name(typename std::remove_reference_t<decltype(member_name)> &&_member_name)\
{\
	member_name = std::move(_member_name);\
}


#define GETSET(func_name, member_name)\
GETTER(func_name, member_name)\
SETTER(func_name, member_name)

