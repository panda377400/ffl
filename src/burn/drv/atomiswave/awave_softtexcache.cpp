#include "awave_wrap_config.h"

#if !defined(HAVE_OPENGL) && !defined(HAVE_OPENGLES)

#include <cstring>
#include <xmmintrin.h>

#include "rend/gles/gles.h"

extern u32 decoded_colors[3][65536];

GlTextureCache TexCache;

static inline u32 SoftPaletteLookup(u8 index)
{
	return palette32_ram[(palette_index + index) & 1023];
}

static inline u32 SoftColor32ToArgb(u32 rgba)
{
	return (rgba & 0xFF000000)
		| ((rgba & 0x000000FF) << 16)
		| (rgba & 0x0000FF00)
		| ((rgba & 0x00FF0000) >> 16);
}

static inline u32 SoftNormalizeTransparent(u32 color)
{
	return (color & 0xFF000000) == 0 ? 0 : color;
}

static inline const u8* SoftTextureBaseLevel(const u8* buffer, int width, int height, TextureType texType, bool mipmapsIncluded)
{
	if (buffer == nullptr || !mipmapsIncluded || width <= 0 || height <= 0) {
		return buffer;
	}

	size_t bytesPerPixel = 2;
	switch (texType)
	{
	case TextureType::_8888:
		bytesPerPixel = 4;
		break;
	case TextureType::_8:
		bytesPerPixel = 1;
		break;
	default:
		bytesPerPixel = 2;
		break;
	}

	size_t offset = 0;
	int mipWidth = 1;
	int mipHeight = 1;

	while (mipWidth < width || mipHeight < height)
	{
		offset += (size_t)mipWidth * (size_t)mipHeight * bytesPerPixel;
		if (mipWidth < width) {
			mipWidth <<= 1;
		}
		if (mipHeight < height) {
			mipHeight <<= 1;
		}
	}

	return buffer + offset;
}

bool TextureCacheData::Delete()
{
	if (!BaseTextureCacheData::Delete())
		return false;

	if (pData != nullptr)
	{
		_mm_free(pData);
		pData = nullptr;
	}

	softWidth = 0;
	softHeight = 0;
	texID = 0;

	return true;
}

void TextureCacheData::UploadToGPU(int width, int height, u8 *temp_tex_buffer, bool mipmapped, bool mipmapsIncluded)
{
	(void)mipmapped;

	if (pData != nullptr)
	{
		_mm_free(pData);
		pData = nullptr;
	}

	softWidth = width;
	softHeight = height;

	if (width == 0 || height == 0 || temp_tex_buffer == nullptr)
		return;

	const u8* texBuffer = SoftTextureBaseLevel(temp_tex_buffer, width, height, tex_type, mipmapsIncluded);
	if (texBuffer == nullptr)
		return;

	pData = (u16*)_mm_malloc((size_t)width * (size_t)height * 16, 16);
	if (pData == nullptr)
		return;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			u32* data = (u32*)&pData[(x + y * width) * 8];
			int x1 = (x + 1) % width;
			int y1 = (y + 1) % height;
			int idx00 = x + y * width;
			int idx01 = x1 + y * width;
			int idx10 = x + y1 * width;
			int idx11 = x1 + y1 * width;

			switch (tex_type)
			{
			case TextureType::_565:
			case TextureType::_5551:
			case TextureType::_4444:
			{
				const u16* tex_data = (const u16*)texBuffer;
				data[0] = SoftNormalizeTransparent(decoded_colors[(int)tex_type][tex_data[idx11]]);
				data[1] = SoftNormalizeTransparent(decoded_colors[(int)tex_type][tex_data[idx10]]);
				data[2] = SoftNormalizeTransparent(decoded_colors[(int)tex_type][tex_data[idx01]]);
				data[3] = SoftNormalizeTransparent(decoded_colors[(int)tex_type][tex_data[idx00]]);
				break;
			}
			case TextureType::_8888:
			{
				const u32* tex_data = (const u32*)texBuffer;
				data[0] = SoftNormalizeTransparent(SoftColor32ToArgb(tex_data[idx11]));
				data[1] = SoftNormalizeTransparent(SoftColor32ToArgb(tex_data[idx10]));
				data[2] = SoftNormalizeTransparent(SoftColor32ToArgb(tex_data[idx01]));
				data[3] = SoftNormalizeTransparent(SoftColor32ToArgb(tex_data[idx00]));
				break;
			}
			case TextureType::_8:
			{
				const u8* tex_data = texBuffer;
				data[0] = SoftNormalizeTransparent(SoftPaletteLookup(tex_data[idx11]));
				data[1] = SoftNormalizeTransparent(SoftPaletteLookup(tex_data[idx10]));
				data[2] = SoftNormalizeTransparent(SoftPaletteLookup(tex_data[idx01]));
				data[3] = SoftNormalizeTransparent(SoftPaletteLookup(tex_data[idx00]));
				break;
			}
			default:
				data[0] = 0;
				data[1] = 0;
				data[2] = 0;
				data[3] = 0;
				break;
			}
		}
	}
}

text_info raw_GetTexture(TSP tsp, TCW tcw)
{
	text_info info = { 0 };
	TextureCacheData* texture_data = TexCache.getTextureCacheData(tsp, tcw);

	if (!texture_data->softInitialized)
	{
		texture_data->Create();
		texture_data->softInitialized = true;
	}

	if (texture_data->NeedsUpdate() || texture_data->pData == nullptr)
		texture_data->Update();

	info.pdata = texture_data->pData;
	info.width = texture_data->softWidth;
	info.height = texture_data->softHeight;
	info.textype = (u32)texture_data->tex_type;

	return info;
}

#endif
