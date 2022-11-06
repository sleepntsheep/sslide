#pragma once
#ifndef ATTR_H
#define ATTR_H

#undef ATTRIBUTE
#ifdef __GNUC__
#define ATTRIBUTE(x) __attribute__((x))
#else
#define ATTRIBUTE(x)
#endif

#undef UNUSED
#define UNUSED(X) ATTRIBUTE(unused) X

#undef FALLTHROUGH
#if __STDC_VERSION__ >= 201710L
#define FALLTHROUGH [[fallthrough]]
#else
#define FALLTHROUGH ((void)0)
#endif

#undef DEPRECATED
#define DEPRECATED ATTRIBUTE(deprecated)

#endif /* ATTR_H */

