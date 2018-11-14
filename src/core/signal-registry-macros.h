#ifndef IRSSI_CORE_SIGNAL_REGISTRY_MACROS_H
#define IRSSI_CORE_SIGNAL_REGISTRY_MACROS_H

/*
 escaping for identifiers to string signals:
   " " -> "_"
   "_" -> "__"
*/
#define SIGNAL_REGISTER_FIX_NAME(VAR) {			\
		char *i, *o ;				\
		for (i = o = VAR; i; i++, o++) {	\
			if (*i == '_') {		\
				if (i[1] == '_') {	\
					i++;		\
					*o = '_';	\
				} else {		\
					*o = ' ';	\
				}			\
			} else {			\
				*o = *i;		\
			}				\
		}					\
		*o = '\0';				\
	}

#define SIGNAL_REGISTER(SIGNAL, NUM, PROTO, ...)			\
	inline static void signal_emit__ ## SIGNAL  PROTO  {		\
		static int SIGNAL ## _id;				\
		if ( ! SIGNAL ## _id ) {				\
			char signal_name[] = #SIGNAL ;			\
			SIGNAL_REGISTER_FIX_NAME(signal_name);		\
			SIGNAL ## _id = signal_get_uniq_id(signal_name); \
		}							\
		signal_emit_id(SIGNAL ## _id , NUM , ## __VA_ARGS__ );	\
	}								\

#define SIGNAL_REGISTER_F(SIGNAL, NUM, PROTO, ARG, ...)			\
	inline static void signal_emit__ ## SIGNAL ## _ PROTO  {	\
		char *signal_name, base_signal_name[] = #SIGNAL;	\
		SIGNAL_REGISTER_FIX_NAME(base_signal_name);		\
		signal_name = g_strconcat(base_signal_name, " ", ARG, NULL); \
		signal_emit(signal_name , NUM , ## __VA_ARGS__ );	\
		g_free(signal_name);					\
	}								\


#define STYPE(TYPE) typedef struct _ ## TYPE TYPE

typedef void *int_in_ptr;
typedef void *mem_ptr;	
typedef struct _GSList GSList;

#endif
