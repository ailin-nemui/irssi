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
									\
	typedef void (*signal_func_ ## SIGNAL ## _t) PROTO ;		\
									\
	inline static void signal_add_full__ ## SIGNAL			\
	(const char *module, int priority,				\
	 signal_func_ ## SIGNAL ## _t func,				\
	 void *user_data) {						\
		char signal_name[] = #SIGNAL ;					\
		signal_register_fix_name(signal_name);			\
		signal_add_full(module, priority, signal_name, (SIGNAL_FUNC) func, user_data); \
	}								\
									\
	inline static void signal_add__ ## SIGNAL (signal_func_ ## SIGNAL ## _t func) { \
		signal_add_full__ ## SIGNAL (MODULE_NAME, SIGNAL_PRIORITY_DEFAULT, func, NULL); \
	}								\
	inline static void signal_add_first__ ## SIGNAL (signal_func_ ## SIGNAL ## _t func) { \
		signal_add_full__ ## SIGNAL (MODULE_NAME, SIGNAL_PRIORITY_HIGH, func, NULL); \
	}								\
	inline static void signal_add_last__ ## SIGNAL (signal_func_ ## SIGNAL ## _t func) { \
		signal_add_full__ ## SIGNAL (MODULE_NAME, SIGNAL_PRIORITY_LOW, func, NULL); \
	}								\
	inline static void signal_add_data__ ## SIGNAL (signal_func_ ## SIGNAL ## _t func, void *data) { \
		signal_add_full__ ## SIGNAL (MODULE_NAME, SIGNAL_PRIORITY_DEFAULT, func, data); \
	}								\
	inline static void signal_add_first_data__ ## SIGNAL (signal_func_ ## SIGNAL ## _t func, void *data) { \
		signal_add_full__ ## SIGNAL (MODULE_NAME, SIGNAL_PRIORITY_HIGH, func, data); \
	}								\
	inline static void signal_add_last_data__ ## SIGNAL (signal_func_ ## SIGNAL ## _t func, void *data) { \
		signal_add_full__ ## SIGNAL (MODULE_NAME, SIGNAL_PRIORITY_LOW, func, data); \
	}								\
	\
	inline static void signal_remove_full__ ## SIGNAL			\
	(signal_func_ ## SIGNAL ## _t func,				\
	 void *user_data) {						\
		char signal_name[] = #SIGNAL ;					\
		signal_register_fix_name(signal_name);			\
		signal_remove_full(signal_name, (SIGNAL_FUNC) func, user_data); \
	}								\
									\
	inline static void signal_remove__ ## SIGNAL (signal_func_ ## SIGNAL ## _t func) { \
		signal_remove_full__ ## SIGNAL (func, NULL); \
	}								\
	inline static void signal_remove_data__ ## SIGNAL (signal_func_ ## SIGNAL ## _t func, void *data) { \
		signal_remove_full__ ## SIGNAL (func, data); \
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
									\
	typedef void (*signal_func_ ## SIGNAL ## __t) PROTO ;		\
									\
	inline static void signal_add_full__ ## SIGNAL ## _		\
	(const char *module, int priority,				\
	 const char *arg, signal_func_ ## SIGNAL ## __t func,		\
	 void *user_data) {						\
		char *t, signal_name[] = #SIGNAL ;			\
		signal_register_fix_name(signal_name);			\
		t = g_strconcat(signal_name, " ", arg, NULL);		\
		signal_add_full(module, priority, t, (SIGNAL_FUNC) func, user_data); \
		g_free(t);						\
	}								\
									\
	inline static void signal_add__ ## SIGNAL ## _ (const char *arg, signal_func_ ## SIGNAL ## __t func) { \
		signal_add_full__ ## SIGNAL ## _ (MODULE_NAME, SIGNAL_PRIORITY_DEFAULT, arg, func, NULL); \
	}								\
	inline static void signal_add_first__ ## SIGNAL ## _ (const char *arg, signal_func_ ## SIGNAL ## __t func) { \
		signal_add_full__ ## SIGNAL ## _ (MODULE_NAME, SIGNAL_PRIORITY_HIGH, arg, func, NULL); \
	}								\
	inline static void signal_add_last__ ## SIGNAL ## _ (const char *arg, signal_func_ ## SIGNAL ## __t func) { \
		signal_add_full__ ## SIGNAL ## _ (MODULE_NAME, SIGNAL_PRIORITY_LOW, arg, func, NULL); \
	}								\
	inline static void signal_add_data__ ## SIGNAL ## _ (const char *arg, signal_func_ ## SIGNAL ## __t func, void *data) { \
		signal_add_full__ ## SIGNAL ## _ (MODULE_NAME, SIGNAL_PRIORITY_DEFAULT, arg, func, data); \
	}								\
	inline static void signal_add_first_data__ ## SIGNAL ## _ (const char *arg, signal_func_ ## SIGNAL ## __t func, void *data) { \
		signal_add_full__ ## SIGNAL ## _ (MODULE_NAME, SIGNAL_PRIORITY_HIGH, arg, func, data); \
	}								\
	inline static void signal_add_last_data__ ## SIGNAL ## _ (const char *arg, signal_func_ ## SIGNAL ## __t func, void *data) { \
		signal_add_full__ ## SIGNAL ## _ (MODULE_NAME, SIGNAL_PRIORITY_LOW, arg, func, data); \
	}								\
									\
	inline static void signal_remove_full__ ## SIGNAL ## _		\
	(const char *arg, signal_func_ ## SIGNAL ## __t func,		\
	 void *user_data) {						\
		char *t, signal_name[] = #SIGNAL ;			\
		signal_register_fix_name(signal_name);			\
		t = g_strconcat(signal_name, " ", arg, NULL);		\
		signal_remove_full(t, (SIGNAL_FUNC) func, user_data);	\
		g_free(t);						\
	}								\
									\
	inline static void signal_remove__ ## SIGNAL ## _ (const char *arg, signal_func_ ## SIGNAL ## __t func) { \
		signal_remove_full__ ## SIGNAL ## _ (arg, func, NULL);	\
	}								\
	inline static void signal_remove_data__ ## SIGNAL ## _ (const char *arg, signal_func_ ## SIGNAL ## __t func, void *data) { \
		signal_remove_full__ ## SIGNAL ## _ (arg, func, data);	\
	}								\

#define STYPE(TYPE) typedef struct _ ## TYPE TYPE

#define SIGNAL_EMIT(SIGNAL, ...) signal_emit__ ## SIGNAL ( ## __VA_ARGS__ )
#define SIGNAL_EMIT_(SIGNAL, ...) signal_emit__ ## SIGNAL ## _ ( ## __VA_ARGS__ )

typedef void *int_in_ptr;
typedef void *mem_ptr;	
typedef struct _GSList GSList;

#endif
