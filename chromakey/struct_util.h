#pragma once
#include "struct_def.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    static inline void matrix4_copy(struct matrix4 *dst, const struct matrix4 *m)
    {
        dst->x.m = m->x.m;
        dst->y.m = m->y.m;
        dst->z.m = m->z.m;
        dst->t.m = m->t.m;
    }

    static inline void matrix4_transpose(struct matrix4 *dst, const struct matrix4 *m)
    {
        if (dst == m) {
            struct matrix4 temp = *m;
            matrix4_transpose(dst, &temp);
            return;
        }

        __m128 a0 = _mm_unpacklo_ps(m->x.m, m->z.m);
        __m128 a1 = _mm_unpacklo_ps(m->y.m, m->t.m);
        __m128 a2 = _mm_unpackhi_ps(m->x.m, m->z.m);
        __m128 a3 = _mm_unpackhi_ps(m->y.m, m->t.m);

        dst->x.m = _mm_unpacklo_ps(a0, a1);
        dst->y.m = _mm_unpackhi_ps(a0, a1);
        dst->z.m = _mm_unpacklo_ps(a2, a3);
        dst->t.m = _mm_unpackhi_ps(a2, a3);
    }

static inline void vec2_set(struct vec2 *dst, float x, float y)
{
    dst->x = x;
    dst->y = y;
}

static inline void vec4_from_rgba(struct vec4 *dst, uint32_t rgba)
{
    dst->x = (float)((double)(rgba & 0xFF) * (1.0 / 255.0));
    rgba >>= 8;
    dst->y = (float)((double)(rgba & 0xFF) * (1.0 / 255.0));
    rgba >>= 8;
    dst->z = (float)((double)(rgba & 0xFF) * (1.0 / 255.0));
    rgba >>= 8;
    dst->w = (float)((double)(rgba & 0xFF) * (1.0 / 255.0));
}

static inline float vec4_dot(const struct vec4 *v1, const struct vec4 *v2)
{
    struct vec4 add;
    __m128 mul = _mm_mul_ps(v1->m, v2->m);
    add.m = _mm_add_ps(_mm_movehl_ps(mul, mul), mul);
    add.m = _mm_add_ps(_mm_shuffle_ps(add.m, add.m, 0x55), add.m);
    return add.x;
}

static inline void vec4_copy(struct vec4 *dst, const struct vec4 *v)
{
    dst->m = v->m;
}

static inline void vec4_transform(struct vec4 *dst, const struct vec4 *v,
    const struct matrix4 *m)
{
    struct vec4 temp;
    struct matrix4 transpose;

    matrix4_transpose(&transpose, m);

    temp.x = vec4_dot(&transpose.x, v);
    temp.y = vec4_dot(&transpose.y, v);
    temp.z = vec4_dot(&transpose.z, v);
    temp.w = vec4_dot(&transpose.t, v);

    vec4_copy(dst, &temp);
}



#ifdef __cplusplus
}
#endif