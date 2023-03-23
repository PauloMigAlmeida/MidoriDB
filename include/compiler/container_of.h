#ifndef COMPILER_CONTAINEROF_H
#define COMPILER_CONTAINEROF_H

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 * WARNING: any const qualifier of @ptr is lost.
 */
#define container_of(ptr, type, member) __extension__({			\
        void *__mptr = (void *)(ptr);					\
	BUILD_BUG(__same_type(*(ptr), ((type *)0)->member) ||	        \
                      __same_type(*(ptr), void),			\
                      "pointer type mismatch in container_of()");	\
        ((type *)(__mptr - offsetof(type, member))); })


#endif /* COMPILER_CONTAINEROF_H */
