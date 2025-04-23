#ifndef MIN_UNIT_H
#define MIN_UNIT_H

#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { mu_setup(); char *message = test(); tests_run++;		\
                                if (message) return message; } while (0)
extern int tests_run;

#endif
