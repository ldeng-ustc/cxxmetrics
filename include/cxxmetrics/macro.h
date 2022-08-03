#ifndef __CXXMETRICS_MACRO__HPP__
#define __CXXMETRICS_MACRO__HPP__

#if defined(__linux__)
  #define CXXMETRICS_LINUX
#endif

#if defined(__clang__)
  #define CXXMETRICS_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
  #define CXXMETRICS_GCC
#elif defined(_MSC_VER)
  #define CXXMETRICS_VS
#endif

#if __cplusplus >= 202002L
#define CXXMETRICS_USE_CONCEPTS
#endif

#ifdef __x86_64__
#define CXXMETRICS_USE_TSC
#endif


#endif