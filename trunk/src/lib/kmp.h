#ifndef _KMP_H_
#define _KMP_H_

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

extern int KMP(const char* strPattern, int len, const char* strTarget);
extern void KMP_end();

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif
