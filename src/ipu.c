// vim: noet ts=4 sw=4 sts=0
#include "ipu.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***************************************************
 *
 * stack
 *
 */

static IpuStack * _S;


IpuStack * ipu_stack_select(IpuStack * S)
{
	IpuStack * old_S = _S;
	_S = S;
	return old_S;
}

void ipu_stack_push(IpuImage * I)
{
	pack_add_tail(_S, I);
}

IpuImage * ipu_stack_pop()
{
	if (ipu_stack_is_empty()) return NULL;
	PackElement * pe = (PackElement *)_S->prev;
	IpuImage * I = (IpuImage *)pe->data;
	pack_delete(pe);
	return I;
}

IpuImage * ipu_stack_top()
{
	if (ipu_stack_is_empty()) return NULL;
	PackElement * pe = (PackElement *)_S->prev;
	return (IpuImage *)pe->data;
}

int ipu_stack_is_empty()
{
	return (pack_length(_S) == 0);
}


/***************************************************
 *
 * image manipulation
 *
 */

IpuImage * ipu_image_new()
{
	create(IpuImage, I);
	return I;
}

void ipu_image_free(IpuImage * I)
{
	free(I);
}


bool ipu_color(float r, float g, float b)
{
	$_(I, ipu_image_new());	// if this fail, let it crash the app!
	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) = r;
			ipu_at(I, x, y, 1) = g;
			ipu_at(I, x, y, 2) = b;
		}
	ipu_stack_push(I);

	return false;
}

bool ipu_pixel(float r, float g, float b, int npoint, int seed)
{
	$_(I, ipu_stack_top());
	if (!I) return true;

	srand(seed);
	while (npoint--) {
		int x = rand();
		int y = rand();
		ipu_at(I, x, y, 0) = r;
		ipu_at(I, x, y, 1) = g;
		ipu_at(I, x, y, 2) = b;
	}

	return false;
}

bool ipu_blur_x(int radius)
{
	$_(I, ipu_stack_pop());
	if (!I) return true;

	$_(new_I, ipu_image_new());	// if this fail, let it crash the app!
	int y, x;
	for (y=0; y<256; y++) {
		float color[4] = {0, 0, 0, radius*2+1};
		for (x=-radius; x<=radius; x++) {
			color[0] += ipu_at(I, x, y, 0);
			color[1] += ipu_at(I, x, y, 1);
			color[2] += ipu_at(I, x, y, 2);
		}
		ipu_at(new_I, 0, y, 0) = color[0] / color[3];
		ipu_at(new_I, 0, y, 1) = color[1] / color[3];
		ipu_at(new_I, 0, y, 2) = color[2] / color[3];
		for (x=1; x<256; x++) {
			ipu_at(new_I, x, y, 0) = ipu_at(new_I, x-1, y, 0) +
				(ipu_at(I, x+radius, y, 0) -
				 ipu_at(I, x-radius-1, y, 0)) / color[3];
			ipu_at(new_I, x, y, 1) = ipu_at(new_I, x-1, y, 1) +
				(ipu_at(I, x+radius, y, 1) -
				 ipu_at(I, x-radius-1, y, 1)) / color[3];
			ipu_at(new_I, x, y, 2) = ipu_at(new_I, x-1, y, 2) +
				(ipu_at(I, x+radius, y, 2) -
				 ipu_at(I, x-radius-1, y, 2)) / color[3];
		}
	}
	ipu_image_free(I);
	ipu_stack_push(new_I);

	return false;
}

bool ipu_blur_y(int radius)
{
	$_(I, ipu_stack_pop());
	if (!I) return true;

	$_(new_I, ipu_image_new());	// if this fail, let it crash the app!
	int y, x;
	for (x=0; x<256; x++) {
		float color[4] = {0, 0, 0, radius*2+1};
		for (y=-radius; y<=radius; y++) {
			color[0] += ipu_at(I, x, y, 0);
			color[1] += ipu_at(I, x, y, 1);
			color[2] += ipu_at(I, x, y, 2);
		}
		ipu_at(new_I, x, 0, 0) = color[0] / color[3];
		ipu_at(new_I, x, 0, 1) = color[1] / color[3];
		ipu_at(new_I, x, 0, 2) = color[2] / color[3];
		for (y=1; y<256; y++) {
			ipu_at(new_I, x, y, 0) = ipu_at(new_I, x, y-1, 0) +
				(ipu_at(I, x, y+radius, 0) -
				 ipu_at(I, x, y-radius-1, 0)) / color[3];
			ipu_at(new_I, x, y, 1) = ipu_at(new_I, x, y-1, 1) +
				(ipu_at(I, x, y+radius, 1) -
				 ipu_at(I, x, y-radius-1, 1)) / color[3];
			ipu_at(new_I, x, y, 2) = ipu_at(new_I, x, y-1, 2) +
				(ipu_at(I, x, y+radius, 2) -
				 ipu_at(I, x, y-radius-1, 2)) / color[3];
		}
	}
	ipu_image_free(I);
	ipu_stack_push(new_I);

	return false;
}

bool ipu_blur(int radius, float angle_in_degree)
{
	return ipu_blur_x(radius * cos(angle_in_degree * M_PI / 180)) ||
			ipu_blur_y(radius * sin(angle_in_degree * M_PI / 180));
}

bool ipu_mul(float r, float g, float b)
{
	$_(I, ipu_stack_top());
	if (!I) return true;

	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) *= r;
			ipu_at(I, x, y, 1) *= g;
			ipu_at(I, x, y, 2) *= b;
		}

	return false;
}

bool ipu_dup()
{
	$_(I, ipu_stack_top());
	if (!I) return true;

	$_(new_I, ipu_image_new());	// if this fail, let it crash the app!
	memcpy(new_I, I, sizeof(IpuImage));
	ipu_stack_push(new_I);

	return false;
}

bool ipu_ignore()
{
	$_(I, ipu_stack_pop());
	if (!I) return true;

	ipu_image_free(I);

	return false;
}

bool ipu_clamp()
{
#define CLAMP(v, f, t) ({ \
	typeof(v) __$v = (v); \
	typeof(f) __$f = (f); \
	typeof(t) __$t = (t); \
	(__$v < __$f ? __$f : (__$v > __$t ? __$t : __$v)); \
})
	$_(I, ipu_stack_top());
	if (!I) return true;

	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) = CLAMP(ipu_at(I, x, y, 0), 0, 1);
			ipu_at(I, x, y, 1) = CLAMP(ipu_at(I, x, y, 1), 0, 1);
			ipu_at(I, x, y, 2) = CLAMP(ipu_at(I, x, y, 2), 0, 1);
		}

	return false;
#undef CLAMP
}

bool ipu_level(float f, float t)
{
#define LIRP(v, f, t, df, dt) ({ \
	typeof(v) __$v = (v); \
	typeof(f) __$f = (f); \
	typeof(t) __$t = (t); \
	typeof(df) __$df = (df); \
	typeof(dt) __$dt = (dt); \
	(__$v-__$f) / (__$t-__$f) * (__$dt-__$df) + __$df; \
})
	$_(I, ipu_stack_top());
	if (!I) return true;

	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) = LIRP(ipu_at(I, x, y, 0), f, t, 0, 1);
			ipu_at(I, x, y, 1) = LIRP(ipu_at(I, x, y, 1), f, t, 0, 1);
			ipu_at(I, x, y, 2) = LIRP(ipu_at(I, x, y, 2), f, t, 0, 1);
		}

	return false;
#undef LIRP
}


bool ipu_mix_add()
{
	$_(I2, ipu_stack_pop());
	if (!I2) return true;

	$_(I, ipu_stack_top());
	if (!I) {
		ipu_image_free(I2);
		return true;
	}

	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) += ipu_at(I2, x, y, 0);
			ipu_at(I, x, y, 1) += ipu_at(I2, x, y, 1);
			ipu_at(I, x, y, 2) += ipu_at(I2, x, y, 2);
		}

	return false;
}

bool ipu_mix_sub()
{
	return true;
}

bool ipu_mix_mul()
{
	return true;
}

bool ipu_mix_div()
{
	return true;
}


/***************************************************
 *
 * file io
 *
 */

bool ipu_save_to_ppm(const char * filename)
{
	FILE * fp = fopen(filename, "w");
	if (!fp) return true;

	if (ipu_dup() || ipu_clamp()) goto __fail;

	$_(I, ipu_stack_pop());
	if (!I) goto __fail;

	fprintf(fp, "P6\n256 256\n255\n");
	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			unsigned char color[3] = {
				ipu_at(I, x, y, 0) * 255,
				ipu_at(I, x, y, 1) * 255,
				ipu_at(I, x, y, 2) * 255,
			};
			fwrite(color, sizeof(color), 1, fp);
		}
	fclose(fp);

	ipu_image_free(I);
	return false;

__fail:
	fclose(fp);
	return true;
}

