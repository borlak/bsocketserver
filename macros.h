#include <stdlib.h>
#include <string.h>

///////////////////////////////
// DATA TYPE HANDLING MACROS //
///////////////////////////////
#define ValidString(str)	((str) && (str[0]) != '\0')
#define Upper(c)			((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))
#define Lower(c)			((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define IsSet(c,bit)		(c & bit)
#define Max(a,b)			(a>b?a:b)
#define Min(a,b)			(a>b?b:a)

////////////////
// XML MACROS //
////////////////
#define XmlName(node,str)	(!strcmp((char*)(node)->name,(str)))
#define XmlInt(node)		((node)->children ? (node)->children->content ? atoi((char*)(node)->children->content) : 0 : 0)
#define XmlLong(node)		((node)->children ? (node)->children->content ? atol((char*)(node)->children->content) : 0 : 0)
#define XmlFloat(node)		((node)->children ? (node)->children->content ? (float)atof((char*)(node)->children->content) : 0.0 : 0.0)
#define XmlStr(node)		((node)->children ? (node)->children->content ? (char*)(node)->children->content : 0 : 0)

////////////////////////
// LINKED LIST MACROS //
////////////////////////
#define NewObject(freelist,obj,TYPE)		\
	if( freelist == 0 ) {					\
		obj = (TYPE*) malloc(sizeof(*obj));	\
		memset(obj,0,sizeof(*obj));			\
	}										\
	else {									\
		obj = freelist;						\
		freelist = freelist->next;			\
		if(freelist)						\
			freelist->prev = 0;				\
		memset(obj,0,sizeof(*obj));			\
	}

#define DeleteObject(obj)	\
	free(obj);				\
	obj = 0;

#define AddToList(list,obj)	\
	if( list != 0 ) {		\
		list->prev = obj;	\
		obj->next = list;	\
		obj->prev = 0;		\
		list = obj;			\
	}						\
	else {					\
		list = obj;			\
		list->prev = list->next = 0;	\
	}

#define AddToListEnd(list,obj,dummy)	\
	if( list != 0 ) {					\
		for(dummy = list; dummy->next; dummy = dummy->next) ;	\
		dummy->next = obj;				\
		obj->prev = dummy;				\
		obj->next = 0;					\
	}									\
	else {								\
		list = obj;						\
		list->prev = list->next = 0;	\
	}

#define RemoveFromList(list,obj)	\
	if( obj->prev == 0 ) {			\
		if( obj->next == 0 ) { 		\
			if(list == obj)			\
				list = 0;			\
		}							\
		else {						\
			list = obj->next;		\
			list->prev = obj->next = 0;	\
		}							\
	}								\
	else {							\
		if( obj->next )				\
			obj->next->prev = obj->prev;	\
		obj->prev->next = obj->next;		\
		obj->next = obj->prev = 0;			\
	}


//////////////////////
// HASH LIST MACROS //
//////////////////////
#define AddToHashList(list,obj,key,hash_key)		\
	if( list[(key)%(hash_key)] != 0 ) {				\
		obj->next_hash = list[(key)%(hash_key)];	\
		obj->prev_hash = 0;							\
		list[(key)%(hash_key)]->prev_hash = obj;	\
		list[(key)%(hash_key)] = obj;				\
	}												\
	else {											\
		list[(key)%(hash_key)] = obj;				\
		obj->next_hash = obj->prev_hash = 0;		\
	}


#define RemoveFromHashList(list,obj,key,hash_key)	\
	if( obj->prev_hash == 0 ) {						\
		if( obj->next_hash == 0 ) {					\
			if(list[(key)%(hash_key)] == obj)		\
				list[(key)%(hash_key)] = 0;			\
		}											\
		else {										\
			list[(key)%(hash_key)] = obj->next_hash;				\
			list[(key)%(hash_key)]->prev_hash = obj->next_hash = 0;	\
		}											\
	}												\
	else {											\
		if( obj->next_hash )						\
			obj->next_hash->prev_hash = obj->prev_hash;		\
		obj->prev_hash->next_hash = obj->next_hash;			\
		obj->next_hash = obj->prev_hash = 0;				\
	}

