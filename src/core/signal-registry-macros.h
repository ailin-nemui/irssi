#ifndef IRSSI_CORE_SIGNAL_REGISTRY_MACROS_H
#define IRSSI_CORE_SIGNAL_REGISTRY_MACROS_H

/*
 escaping for identifiers to string signals:
   " " -> "_"
   "_" -> "__"
*/
inline static void signal_register_fix_name(char *var) {
		char *i, *o ;
		for (i = o = var; *i; i++, o++) {
			if (*i == '_') {
				if (i[1] == '_') {
					i++;
					*o = '_';
				} else {
					*o = ' ';
				}
			} else {
				*o = *i;
			}
		}
		*o = '\0';
	}

#define SIGNAL_REGISTER(SIGNAL, NUM, PROTO, ...)			\
	inline static int signal_emit__ ## SIGNAL  PROTO  {		\
		static int SIGNAL ## _id;				\
		if ( ! SIGNAL ## _id ) {				\
			char signal_name[] = #SIGNAL ;			\
			signal_register_fix_name(signal_name);		\
			SIGNAL ## _id = signal_get_uniq_id(signal_name); \
		}							\
		return signal_emit_id_raw(SIGNAL ## _id , NUM , ## __VA_ARGS__ ); \
	}								\

#define SIGNAL_REGISTER_F(SIGNAL, NUM, PROTO, ARG, ...)			\
	inline static int signal_emit__ ## SIGNAL ## _ PROTO  {	\
		int ret;						\
		char *signal_name, base_signal_name[] = #SIGNAL;	\
		signal_register_fix_name(base_signal_name);		\
		signal_name = g_strconcat(base_signal_name, " ", ARG, NULL); \
		ret = signal_emit_raw(signal_name , NUM , ## __VA_ARGS__ ); \
		g_free(signal_name);					\
		return ret;						\
	}								\


#define STYPE(TYPE) typedef struct _ ## TYPE TYPE

typedef void *int_in_ptr;
typedef void *mem_ptr;	
typedef struct _GSList GSList;

#endif
