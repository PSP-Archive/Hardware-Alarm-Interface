#include <stdlib.h>
#include <malloc.h>
#include <pspdisplay.h>
#include <psputils.h>
#include <png.h>
#include <pspgu.h>

#include "graphics.h"
#include "framebuffer.h"
#include <vram.h>

#define IS_ALPHA(color) (((color)&0xff000000)==0xff000000?0:1)
#define FRAMEBUFFER_SIZE (PSP_LINE_SIZE*SCREEN_HEIGHT*4)
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
extern u8 msx[];

unsigned int __attribute__((aligned(16))) list[262144];

static int dispBufferNumber;
static int initialized = 0;

static int getNextPower2(int width)
{
	int b = width;
	int n;
	for (n = 0; b != 0; n++) b >>= 1;
	b = 1 << n;
	if (b == 2 * width) b >>= 1;
	return b;
}

Color* getVramDrawBuffer()
{
	Color* vram = (Color*) g_vram_base;
	if (dispBufferNumber == 0) vram += FRAMEBUFFER_SIZE / sizeof(Color);
	return vram;
}

Color* getVramDisplayBuffer()
{
	Color* vram = (Color*) g_vram_base;
	if (dispBufferNumber == 1) vram += FRAMEBUFFER_SIZE / sizeof(Color);
	return vram;
}

static void read_data_memory(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
	MEMORY_READER_STATE *f = (MEMORY_READER_STATE *)png_get_io_ptr(png_ptr);
	if (length > (f->bufsize - f->current_pos)) png_error(png_ptr, "read error in read_data_memory (loadpng)");
	memcpy(data, f->buffer + f->current_pos, length);
	f->current_pos += length;
}

void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
}

Image* loadImage(const char* filename)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type, x, y;
	u32* line;
	FILE *fp;

	Image* image = (Image*) malloc(sizeof(Image));
	if (!image) return NULL;

	if ((fp = fopen(filename, "rb")) == NULL) return NULL;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		free(image);
		fclose(fp);
		return NULL;;
	}
	png_set_error_fn(png_ptr, (png_voidp) NULL, (png_error_ptr) NULL, user_warning_fn);
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);
	/*
	if (width > 512 || height > 512) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	*/
	image->imageWidth = width;
	image->imageHeight = height;
	image->textureWidth = getNextPower2(width);
	image->textureHeight = getNextPower2(height);
	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	image->data = (Color*) memalign(16, image->textureWidth * image->textureHeight * sizeof(Color));
	if (!image->data) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	line = (u32*) malloc(width * 4);
	if (!line) {
		free(image->data);
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	for (y = 0; y < height; y++) {
		png_read_row(png_ptr, (u8*) line, png_bytep_NULL);
		for (x = 0; x < width; x++) {
			u32 color = line[x];
			image->data[x + y * image->textureWidth] =  color;
		}
	}
	free(line);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
	fclose(fp);

	image->swizzled = 0;
    sceKernelDcacheWritebackAll();
	swizzle(image);
	return image;
}

Image* loadImageMemory(const void *buffer, int bufsize)
{
	png_structp png_ptr;
	png_infop info_ptr;
	MEMORY_READER_STATE memory_reader_state;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type, x, y;
	u32* line;

	if (!buffer || (bufsize <= 0)) return NULL;

	Image* image = (Image*) malloc(sizeof(Image));
	if (!image) return NULL;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		free(image);
		return NULL;;
	}

	png_set_error_fn(png_ptr, (png_voidp) NULL, (png_error_ptr) NULL, user_warning_fn);

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		free(image);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}

	memory_reader_state.buffer = (unsigned char *)buffer;
	memory_reader_state.bufsize = bufsize;
	memory_reader_state.current_pos = 0;

	png_set_read_fn(png_ptr, &memory_reader_state, (png_rw_ptr)read_data_memory);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	image->imageWidth = width;
	image->imageHeight = height;
	image->textureWidth = getNextPower2(width);
	image->textureHeight = getNextPower2(height);

	png_set_sig_bytes(png_ptr, sig_read);
	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

	image->data = (Color*) memalign(16, image->textureWidth * image->textureHeight * sizeof(Color));
	if (!image->data) {
		free(image);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	line = (u32*) malloc(width * 4);
	if (!line) {
		free(image->data);
		free(image);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	for (y = 0; y < height; y++) {
		png_read_row(png_ptr, (u8*) line, png_bytep_NULL);
		for (x = 0; x < width; x++) {
			u32 color = line[x];
			image->data[x + y * image->textureWidth] =  color;
		}
	}

	free(line);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

	image->swizzled = 0;
    sceKernelDcacheWritebackAll();
	swizzle(image);
	return image;
}

void blitImageToImage(int sx, int sy, int width, int height, Image* source, int dx, int dy, Image* destination)
{
	Color* destinationData = &destination->data[destination->textureWidth * dy + dx];
	int destinationSkipX = destination->textureWidth - width;
	Color* sourceData = &source->data[source->textureWidth * sy + sx];
	int sourceSkipX = source->textureWidth - width;
	int x, y;
	for (y = 0; y < height; y++, destinationData += destinationSkipX, sourceData += sourceSkipX) {
		for (x = 0; x < width; x++, destinationData++, sourceData++) {
			*destinationData = *sourceData;
		}
	}
}

void blitImageToScreen(int sx, int sy, int width, int height, Image* source, int dx, int dy)
{
	if (!initialized) return;
	Color* vram = getVramDrawBuffer();
	sceKernelDcacheWritebackInvalidateAll();
	sceGuCopyImage(GU_PSM_8888, sx, sy, width, height, source->textureWidth, source->data, dx, dy, PSP_LINE_SIZE, vram);
}

void blitAlphaImageToImage(int sx, int sy, int width, int height, Image* source, int dx, int dy, Image* destination)
{
	// TODO Blend!
	Color* destinationData = &destination->data[destination->textureWidth * dy + dx];
	int destinationSkipX = destination->textureWidth - width;
	Color* sourceData = &source->data[source->textureWidth * sy + sx];
	int sourceSkipX = source->textureWidth - width;
	int x, y;
	for (y = 0; y < height; y++, destinationData += destinationSkipX, sourceData += sourceSkipX) {
		for (x = 0; x < width; x++, destinationData++, sourceData++) {
			Color color = *sourceData;
			if (!IS_ALPHA(color)) *destinationData = color;
		}
	}
}

void blitAlphaImageToScreen(float sx, float sy, float width, float height, Image* source, float dx, float dy)
{
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	if (!initialized) return;

	sceGuTexImage(0, source->textureWidth, source->textureHeight, source->textureWidth, (void*) source->data);
	float u = 1.0f / (source->textureWidth);
	float v = 1.0f / (source->textureHeight);
	sceGuTexScale(u, v);

	float j  = 0.0f;
	while (j < width) {
		Vertex* vertices = (Vertex*) sceGuGetMemory(2 * sizeof(Vertex));
		int sliceWidth = 64;
		if (j + sliceWidth > width) sliceWidth = width - j;
		vertices[0].u = sx + j;
		vertices[0].v = sy;
		vertices[0].x = dx + j;
		vertices[0].y = dy;
		vertices[0].z = 0;
		vertices[1].u = sx + j + sliceWidth;
		vertices[1].v = sy + height;
		vertices[1].x = dx + j + sliceWidth;
		vertices[1].y = dy + height;
		vertices[1].z = 0;
		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
		j += sliceWidth;
	}
	}

void RenderImage(Image* pImg,float u0,float v0,float u1,float v1,float x_pos,float y_pos,float width,float height, int soften)
{
// @param x_pos, y_pos, width, height : where to draw the image to and how big it is on screen
	// @param u0, v0, u1, v1 : the image part which is to be drawn, (0,0,imagewidth,imageheight) for the whole image

	if (soften==1) sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	sceGuTexImage(0, pImg->textureWidth, pImg->textureHeight, pImg->textureWidth, (void*) pImg->data);
	float start, end;
	float cur_u = u0;
	float cur_x = x_pos;
	float x_end = x_pos + width;
	float slice = 64.f;
	float ustep = (u1-u0)/width * slice; //when using ints
	//float ustep = (u1-u0)/width * slice;

	// blit maximizing the use of the texture-cache
	for( start=0, end=width; start<end; start+=slice )
		{
		struct VertexNoColor* vertices = (struct VertexNoColor*)sceGuGetMemory(2 * sizeof(struct VertexNoColor));

		float poly_width = ((cur_x+slice) > x_end) ? (x_end-cur_x) : slice;
		float source_width = ((cur_u+ustep) > u1) ? (u1-cur_u) : ustep;

		vertices[0].u = cur_u;
		vertices[0].v = v0;
		vertices[0].x = cur_x;
		vertices[0].y = y_pos;
		vertices[0].z = 0;

		cur_u += source_width;
		cur_x += poly_width;

		vertices[1].u = cur_u;
		vertices[1].v = v1;
		vertices[1].x = cur_x;
		vertices[1].y = (y_pos + height);
		vertices[1].z = 0;

		sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,vertices);
		}
		//if (soften==1) sceGuTexFilter(GU_NEAREST, GU_NEAREST);
}

Image* createImage(int width, int height)
{
	Image* image = (Image*) malloc(sizeof(Image));
	if (!image) return NULL;
	image->imageWidth = width;
	image->imageHeight = height;
	image->textureWidth = getNextPower2(width);
	image->textureHeight = getNextPower2(height);
	image->data = (Color*) memalign(16, image->textureWidth * image->textureHeight * sizeof(Color));
	if (!image->data) return NULL;
	memset(image->data, 0, image->textureWidth * image->textureHeight * sizeof(Color));
	return image;
}

void freeImage(Image* image)
{
	free(image->data);
	free(image);
}

void clearImage(Color color, Image* image)
{
	int i;
	int size = image->textureWidth * image->textureHeight;
	Color* data = image->data;
	for (i = 0; i < size; i++, data++) *data = color;
}

inline void blit(Image* img,float x, float y)
{
blitAlphaImageToScreen(0,0,img->imageWidth ,img->imageHeight,img,x,y);
}

 inline void blitresize(Image* img,float x,float y,float width,float height,int soften)
{
    RenderImage(img,0,0,(float)img->imageWidth,(float)img->imageHeight,(float)x,(float)y,(float)width,(float)height,soften);
}

inline void blitcrop(Image* img,float x,float y,float width,float height)
{
   RenderImage(img,0,0,(float)width,(float)height,(float)x,(float)y,(float)width,(float)height,0);
}

void clearScreen(Color color)
{
	if (!initialized) return;
	sceGuClearDepth(0);
	sceGuClearColor(color);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
	}

void fillImageRect(Color color, int x0, int y0, int width, int height, Image* image)
{
	int skipX = image->textureWidth - width;
	int x, y;
	Color* data = image->data + x0 + y0 * image->textureWidth;
	for (y = 0; y < height; y++, data += skipX) {
		for (x = 0; x < width; x++, data++) *data = color;
	}
}

void fillScreenRect(Color color, int x0, int y0, int width, int height)
{
	if (!initialized) return;
	int skipX = PSP_LINE_SIZE - width;
	int x, y;
	Color* data = getVramDrawBuffer() + x0 + y0 * PSP_LINE_SIZE;
	for (y = 0; y < height; y++, data += skipX) {
		for (x = 0; x < width; x++, data++) *data = color;
	}
}

void drawQuad(int x, int y, int width, int height, unsigned int color)
{
	sceGuDisable(GU_TEXTURE_2D);

	CSVertex* vertices = (CSVertex*) sceGuGetMemory(2 * sizeof(CSVertex));
    sceGuColor(color);
	//vertices[0].color = color;
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0;

	//vertices[1].color = color;
	vertices[1].x = x + width;
	vertices[1].y = y + height;
	vertices[1].z = 0;

	sceGuDrawArray(GU_SPRITES, /*GU_COLOR_8888 |*/ GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuColor(0xFFFFFFFF);
	}

void putPixelScreen(Color color, int x, int y)
{
	  sceGuDisable(GU_TEXTURE_2D);
	Color* vram = getVramDrawBuffer();
	vram[PSP_LINE_SIZE * y + x] = color;
	sceGuEnable(GU_TEXTURE_2D);
}

void putPixelImage(Color color, int x, int y, Image* image)
{
	image->data[x + y * image->textureWidth] = color;
}

Color getPixelScreen(int x, int y)
{
	Color* vram = getVramDrawBuffer();
	return vram[PSP_LINE_SIZE * y + x];
}

Color getPixelImage(int x, int y, Image* image)
{
	return image->data[x + y * image->textureWidth];
}

void printTextScreen(int x, int y, const char* text, u32 color)
{
	int c, i, j, l;
	u8 *font;
	Color *vram_ptr;
	Color *vram;

	if (!initialized) return;

	for (c = 0; c < strlen(text); c++) {
		if (x < 0 || x + 8 > SCREEN_WIDTH || y < 0 || y + 8 > SCREEN_HEIGHT) break;
		char ch = text[c];
		vram = getVramDrawBuffer() + x + y * PSP_LINE_SIZE;

		font = &msx[ (int)ch * 8];
		for (i = l = 0; i < 8; i++, l += 8, font++) {
			vram_ptr  = vram;
			for (j = 0; j < 8; j++) {
				if ((*font & (128 >> j))) *vram_ptr = color;
				vram_ptr++;
			}
			vram += PSP_LINE_SIZE;
		}
		x += 8;
	}
}

void printTextImage(int x, int y, const char* text, u32 color, Image* image)
{
	int c, i, j, l;
	u8 *font;
	Color *data_ptr;
	Color *data;

	if (!initialized) return;

	for (c = 0; c < strlen(text); c++) {
		if (x < 0 || x + 8 > image->imageWidth || y < 0 || y + 8 > image->imageHeight) break;
		char ch = text[c];
		data = image->data + x + y * image->textureWidth;

		font = &msx[ (int)ch * 8];
		for (i = l = 0; i < 8; i++, l += 8, font++) {
			data_ptr  = data;
			for (j = 0; j < 8; j++) {
				if ((*font & (128 >> j))) *data_ptr = color;
				data_ptr++;
			}
			data += image->textureWidth;
		}
		x += 8;
	}
}

void saveImage(const char* filename, Color* data, int width, int height, int lineSize, int saveAlpha)
{
	png_structp png_ptr;
	png_infop info_ptr;
	FILE* fp;
	int i, x, y;
	u8* line;

	if ((fp = fopen(filename, "wb")) == NULL) return;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) return;
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return;
	}
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8,
		saveAlpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	line = (u8*) malloc(width * (saveAlpha ? 4 : 3));
	for (y = 0; y < height; y++) {
		for (i = 0, x = 0; x < width; x++) {
			Color color = data[x + y * lineSize];
			u8 r = color & 0xff;
			u8 g = (color >> 8) & 0xff;
			u8 b = (color >> 16) & 0xff;
			u8 a = saveAlpha ? (color >> 24) & 0xff : 0xff;
			line[i++] = r;
			line[i++] = g;
			line[i++] = b;
			if (saveAlpha) line[i++] = a;
		}
		png_write_row(png_ptr, line);
	}
	free(line);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fclose(fp);
}

void* flipScreen()
{
	if (!initialized) return;
	dispBufferNumber ^= 1;
	return sceGuSwapBuffers();
}

int GuDrawLine(float x1, float y1, float x2, float y2, unsigned int color){

   sceGuDisable(GU_TEXTURE_2D);
   sceGuTexFilter(GU_LINEAR, GU_LINEAR);

   colorVertex* DisplayVertices = (colorVertex*) sceGuGetMemory(2 * sizeof(colorVertex));

   DisplayVertices[0].color = color;
   DisplayVertices[0].x = x1;
   DisplayVertices[0].y = y1;
   DisplayVertices[0].z = 0;

   DisplayVertices[1].color = color;
   DisplayVertices[1].x = x2;
   DisplayVertices[1].y = y2;
   DisplayVertices[1].z = 0;

   sceGuDrawArray(GU_LINES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, DisplayVertices);
   sceGuEnable(GU_TEXTURE_2D);
   return 0;
}


static void drawLine(int x0, int y0, int x1, int y1, int color, Color* destination, int width)
{
	int dy = y1 - y0;
	int dx = x1 - x0;
	int stepx, stepy;

	if (dy < 0) { dy = -dy;  stepy = -width; } else { stepy = width; }
	if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
	dy <<= 1;
	dx <<= 1;

	y0 *= width;
	y1 *= width;
	destination[x0+y0] = color;
	if (dx > dy) {
		int fraction = dy - (dx >> 1);
		while (x0 != x1) {
			if (fraction >= 0) {
				y0 += stepy;
				fraction -= dx;
			}
			x0 += stepx;
			fraction += dy;
			destination[x0+y0] = color;
		}
	} else {
		int fraction = dx - (dy >> 1);
		while (y0 != y1) {
			if (fraction >= 0) {
				x0 += stepx;
				fraction -= dy;
			}
			y0 += stepy;
			fraction += dx;
			destination[x0+y0] = color;
		}
	}
}

void drawLineScreen(int x0, int y0, int x1, int y1, Color color)
{
	drawLine(x0, y0, x1, y1, color, getVramDrawBuffer(), PSP_LINE_SIZE);
}

void drawLineImage(int x0, int y0, int x1, int y1, Color color, Image* image)
{
	drawLine(x0, y0, x1, y1, color, image->data, image->textureWidth);
}

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)
#define PIXEL_SIZE (4) /* change this if you change to another screenmode */
#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
#define ZBUF_SIZE (BUF_WIDTH SCR_HEIGHT * 2) /* zbuffer seems to be 16-bit? */

void moveImageToVram( Image* img)
{
	int size = img->textureHeight * img->textureWidth * 4;
	u32* temp = (u32*)valloc( size);
	if( temp != NULL)
	{
		memcpy( (u8*)((u32)temp|0x40000000), (u8*)img->data, size);
		free( img->data);
		img->invram = 1;
		img->data = temp;
	}
}

void initGraphics()
{
	dispBufferNumber = 0;

	sceGuInit();

	sceGuStart(GU_DIRECT,list);
	//guStart();
	//sceGuDrawBuffer(GU_PSM_8888, (void*)FRAMEBUFFER_SIZE, PSP_LINE_SIZE);
	//sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, (void*)0, PSP_LINE_SIZE);
	void * gu_backbuffer = vrelptr( valloc(FRAMEBUFFER_SIZE));
    void * gu_frontbuffer = vrelptr( valloc(FRAMEBUFFER_SIZE));
    sceGuDrawBuffer( GU_PSM_8888, gu_frontbuffer, PSP_LINE_SIZE);
    sceGuDispBuffer( 480, 272, gu_backbuffer, PSP_LINE_SIZE);

    sceGuDepthBuffer((void*) (FRAMEBUFFER_SIZE*2), PSP_LINE_SIZE);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT);

	sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
	sceGuDepthRange(0xc350, 0x2710);
	sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuAlphaFunc(GU_GREATER, 0, 0xff);
	sceGuEnable(GU_ALPHA_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuDisable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuTexMode(GU_PSM_8888,0,0,GU_TRUE);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	sceGuAmbientColor(0xffffffff);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);// _ALPHA
	sceGuFinish();
	sceGuSync(0, 0);

	sceGuDisplay(GU_TRUE);
	initialized = 1;
}

void disableGraphics()
{
	initialized = 0;
}

void guStart()
{
	sceGuStart(GU_DIRECT, list);
}

void swizzle( Image* img )
{
	if( img->swizzled==0)
	{
		int type = 4;		// 32 - bit pixel format so 4 bytes
		long size = img->textureWidth * img->textureHeight * type;
		u8* temp = (u8*)malloc(size);

		swizzle_fast( temp, (u8*)img->data, (img->textureWidth * type), img->textureHeight );

  		free(img->data);
  		img->data = (u32*)temp;
  		img->swizzled = 1;

	}
}

void swizzle_fast(u8* out, const u8* in, unsigned int width, unsigned int height)
{

   unsigned int blockx, blocky;
   unsigned int i,j;

   unsigned int width_blocks = (width / 16);
   unsigned int height_blocks = (height / 8);

   unsigned int src_pitch = (width-16)/4;
   unsigned int src_row = width * 8;

   const u8* ysrc = in;
   u32* dst = (u32*)out;


      for (blocky = 0; blocky < height_blocks; ++blocky)
   {
      const u8* xsrc = ysrc;
      for (blockx = 0; blockx < width_blocks; ++blockx)
      {
         const u32* src = (u32*)xsrc;
         for (j = 0; j < 8; ++j)
         {
            *(dst++) = *(src++);
            *(dst++) = *(src++);
            *(dst++) = *(src++);
            *(dst++) = *(src++);
            src += src_pitch;
         }
         xsrc += 16;
     }
     ysrc += src_row;
   }
   sceKernelDcacheWritebackAll();
}
