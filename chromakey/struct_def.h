#pragma once
#include <xmmintrin.h>


#ifdef __cplusplus
extern "C" {
#endif

    struct vec2 {
        union {
            struct {
                float x, y;
            };
            float ptr[2];
        };
    };

    struct vec3 {
        union {
            struct {
                float x, y, z, w;
            };
            float ptr[4];
            __m128 m;
        };
    };


    struct vec4 {
        union {
            struct {
                float x, y, z, w;
            };
            float ptr[4];
            __m128 m;
        };
    };

    struct matrix4 {
        struct vec4 x, y, z, t;
    };

#ifdef __cplusplus
}
#endif